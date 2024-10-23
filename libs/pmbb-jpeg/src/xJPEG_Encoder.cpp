/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Encoder.h"
#include "xJPEG_Transform.h"
#include "xJPEG_TransformConstants.h"
#include "xJPEG_Scan.h"
#include "xJPEG_Entropy.h"
#include "xMemory.h"
#include "xPixelOps.h"
#include "xDistortion.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

void xAdvancedEncoder::create(int32V2 PictureSize, eCrF ChromaFormat)
{
  initCodecCommon(PictureSize, ChromaFormat);

  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 Area = m_MCUsMulWidth[CmpIdx] * m_MCUsMulHeight[CmpIdx];
    m_CmpCoeffsTransOrg[CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
    m_CmpCoeffsTransRec[CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
    m_CmpCoeffsScan    [CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
    m_CmpCoeffsScanAux [CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
    m_CmpCoeffsScanOpt [CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
  }

  m_PicRec.create(PictureSize, 8, ChromaFormat, 16);
  m_EntropyBuffer.create(256);
}
void xAdvancedEncoder::destroy()
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    xMemory::xAlignedFreeNull(m_CmpCoeffsTransOrg[CmpIdx]);
    xMemory::xAlignedFreeNull(m_CmpCoeffsTransRec[CmpIdx]);
    xMemory::xAlignedFreeNull(m_CmpCoeffsScan    [CmpIdx]);
    xMemory::xAlignedFreeNull(m_CmpCoeffsScanAux [CmpIdx]);
    xMemory::xAlignedFreeNull(m_CmpCoeffsScanOpt [CmpIdx]);
  }

  m_EntropyBuffer.destroy();
  m_PicRec.destroy();
}
void xAdvancedEncoder::initBaseMarkers()
{
  //init markers
  m_APP0.InitDefault();
  m_SOF0.Init(m_PictureHeight, m_PictureWidth, 8, m_ChromaFormat, 2);
  m_SOS .Init(m_SOF0.getNumComponents(), 0, 1);
}
void xAdvancedEncoder::initQuant(int32 Quality, eQTLa QuantTabLayout)
{
  m_Quality = Quality;

  //init markers
  m_QT.resize(2);
  m_QT[0].Init(0, eCmp::LM, Quality, QuantTabLayout);
  m_QT[1].Init(1, eCmp::CB, Quality, QuantTabLayout); //any chroma so use CB

  if(m_VerboseLevel >= 6)
  {
    std::string Dump = fmt::format("QuantizationTablesMain Num={}\n", m_QT.size());
    for(int32 i = 0; i < (int32)m_QT.size(); i++) { Dump += fmt::format("  QuantTab_{:d}\n", i) + m_QT[i].Format("    "); }
    fmt::print(Dump);
  }

  //init toolbox
  m_QuantMain.Init(m_QT);
  m_QuantAuxD.Init(0, eCmp::LM, Quality - 2, QuantTabLayout);
  m_QuantAuxD.Init(1, eCmp::CB, Quality - 2, QuantTabLayout); //any chroma so use CB
  m_QuantAuxI.Init(0, eCmp::LM, Quality + 2, QuantTabLayout);
  m_QuantAuxI.Init(1, eCmp::CB, Quality + 2, QuantTabLayout); //any chroma so use CB

  if(m_VerboseLevel >= 6)
  {
    std::string Dump = fmt::format("QuantizerTablesMain\n");
    for(int32 i = 0; i < (int32)m_QT.size(); i++)
    {
      Dump += fmt::format("  Table_{}\n", i) + m_QuantMain.getQuantizer(i).FormatCoeffs("    ");
    }
    Dump += fmt::format("QuantizerTablesAux\n");
    for(int32 i = 0; i < (int32)m_QT.size(); i++)
    {
      Dump += fmt::format("  Table_{}\n", i) + m_QuantAuxD.getQuantizer(i).FormatCoeffs("    ");
    }
    fmt::print(Dump);
  }
}
void xAdvancedEncoder::initEntropy(int32 RestartInterval)
{
  //init markers
  m_RestartInterval = RestartInterval;
  m_HT.resize(4);
  m_HT[0].InitDefault(0, xJFIF::xHuffTable::eHuffClass::DC, eCmp::LM);
  m_HT[1].InitDefault(0, xJFIF::xHuffTable::eHuffClass::AC, eCmp::LM);
  m_HT[2].InitDefault(1, xJFIF::xHuffTable::eHuffClass::DC, eCmp::CB); //any chroma so use CB
  m_HT[3].InitDefault(1, xJFIF::xHuffTable::eHuffClass::AC, eCmp::CB); //any chroma so use CB

  //init toolbox
  m_EntropyEnc.Init(m_HT);
  m_EntropyEst.Init(m_HT);

  m_NumMCUsInSlice = RestartInterval != 0 ? RestartInterval : m_NumMCUsInArea;
  int32 NumBlocksInMCU = 0;
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++) { NumBlocksInMCU += m_SampFactorHor[CmpIdx] * m_SampFactorVer[CmpIdx]; }
  int32 MaxEncodedSliceSize = m_NumMCUsInSlice * NumBlocksInMCU * c_BA * 2;
  m_EntropyBuffer.resize(MaxEncodedSliceSize);
}
void xAdvancedEncoder::setMarkerEmit(bool EmitAPP0, bool EmitQuantTabs, bool EmitHuffmanTabs)
{
  m_EmitAPP0      = EmitAPP0       ;
  m_EmitQuantTabs = EmitQuantTabs  ;
  m_EmitHuffTabs  = EmitHuffmanTabs;
}
void xAdvancedEncoder::setRDOQ(bool OptimizeLuma, bool OptimizeChroma, bool ProcessZeroCoeffs, int32 NumBlockOptPasses)
{ 
  m_UseRDOQ           = (OptimizeLuma || OptimizeChroma) && NumBlockOptPasses>0;
  m_OptimizeLuma      = OptimizeLuma;
  m_OptimizeChroma    = OptimizeChroma;
  m_ProcessZeroCoeffs = ProcessZeroCoeffs;
  m_NumBlockOptPasses = NumBlockOptPasses;
  if (m_UseRDOQ) { m_EntropyEst.Init(m_HT); }
}
void xAdvancedEncoder::encode(const xPicYUV* InputPicture, xByteBuffer* OutputBuffer)
{
  xJFIF::WriteSOI (OutputBuffer);
  if(m_EmitAPP0       ) { xJFIF::WriteAPP0(OutputBuffer, m_APP0); }
  if(m_EmitQuantTabs  ) { xJFIF::WriteDQT(OutputBuffer, m_QT); }
  if(m_RestartInterval) { xJFIF::WriteDRI(OutputBuffer, m_RestartInterval); }
  xJFIF::WriteSOF0(OutputBuffer, m_SOF0);
  if(m_EmitHuffTabs   ) { xJFIF::WriteDHT(OutputBuffer, m_HT); }
  xJFIF::WriteSOS (OutputBuffer, m_SOS);
  xEncodePicture(OutputBuffer, InputPicture);
  xJFIF::WriteEOI(OutputBuffer);
}
xAdvancedEncoder::tDistBits xAdvancedEncoder::calcDistBits(const xPicYUV* Picture)
{
  const int16* ConstCmpCoeffsTransOrg[] = { m_CmpCoeffsTransOrg[0], m_CmpCoeffsTransOrg[1], m_CmpCoeffsTransOrg[2], m_CmpCoeffsTransOrg[3] };
  const int16* ConstCmpCoeffsTransRec[] = { m_CmpCoeffsTransRec[0], m_CmpCoeffsTransRec[1], m_CmpCoeffsTransRec[2], m_CmpCoeffsTransRec[3] };
  const int16* ConstCmpCoeffsScan    [] = { m_CmpCoeffsScan    [0], m_CmpCoeffsScan    [1], m_CmpCoeffsScan    [2], m_CmpCoeffsScan    [3] };

  xFwdTransformPic(m_CmpCoeffsTransOrg, Picture);
  xFwdQuantScanPic(m_CmpCoeffsScan    , ConstCmpCoeffsTransOrg, m_QuantMain);
  xInvScanQuantPic(m_CmpCoeffsTransRec, ConstCmpCoeffsScan    , m_QuantMain);
  xInvTransformPic(&m_PicRec          , ConstCmpCoeffsTransRec);
  int64V4 EstNumBits = xHuffEstPic (ConstCmpCoeffsScan);
  int64V4 Distortion = xCalcPicSSDs(Picture, &m_PicRec);

  return std::make_tuple(Distortion, EstNumBits);
}
std::string xAdvancedEncoder::formatAndResetStats(const std::string Prefix)
{
  if(m_TotalPictureIters == 0) { return std::string(); }

  std::string Result = std::string(); Result.reserve(xMemory::c_MemSizePageBase);

  tDurationUS AvgPictureTime   = std::chrono::duration_cast<tDurationUS>(m_TotalPictureTime  ) / m_TotalPictureIters;
  tDurationUS AvgTransformTime = std::chrono::duration_cast<tDurationUS>(m_TotalTransformTime) / m_TotalPictureIters;
  tDurationUS AvgQuantScanTime = std::chrono::duration_cast<tDurationUS>(m_TotalQuantScanTime) / m_TotalPictureIters;
  tDurationUS AvgLambdaTime    = std::chrono::duration_cast<tDurationUS>(m_TotalLambdaTime   ) / m_TotalPictureIters;
  tDurationUS AvgOptimizeTime  = std::chrono::duration_cast<tDurationUS>(m_TotalOptimizeTime ) / m_TotalPictureIters;
  tDurationUS AvgEntropyTime   = std::chrono::duration_cast<tDurationUS>(m_TotalEntropyTime  ) / m_TotalPictureIters;
  tDurationUS AvgStuffingTime  = std::chrono::duration_cast<tDurationUS>(m_TotalStuffingTime ) / m_TotalPictureIters;

  Result += Prefix;
  Result += fmt::format("PicIters={} "          , m_TotalPictureIters);
  Result += fmt::format("PicT={:.0f}us "        , AvgPictureTime  .count());
  Result += fmt::format("TrnsT={:.0f}us "       , AvgTransformTime.count());
  Result += fmt::format("QuantScanT={:.0f}us "  , AvgQuantScanTime.count());
  Result += fmt::format("LmbdT={:.0f}us "       , AvgLambdaTime   .count());
  Result += fmt::format("RateDistOptT={:.0f}us ", AvgOptimizeTime .count());
  Result += fmt::format("EntrT={:.0f}us "       , AvgEntropyTime  .count());
  Result += fmt::format("StffT={:.0f}us "       , AvgStuffingTime .count());
  
  m_TotalPictureTime   = (tDuration)0;
  m_TotalTransformTime = (tDuration)0;
  m_TotalQuantScanTime = (tDuration)0;
  m_TotalLambdaTime    = (tDuration)0;
  m_TotalOptimizeTime  = (tDuration)0;
  m_TotalEntropyTime   = (tDuration)0;
  m_TotalStuffingTime  = (tDuration)0;

  m_TotalPictureIters = 0;
  m_TotalSliceIters   = 0;

  return Result;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void xAdvancedEncoder::xEncodePicture(xByteBuffer* Buffer, const xPicYUV* Picture)
{
  const int16* ConstCmpCoeffsTransOrg[] = { m_CmpCoeffsTransOrg[0], m_CmpCoeffsTransOrg[1], m_CmpCoeffsTransOrg[2], m_CmpCoeffsTransOrg[3] };
  const int16* ConstCmpCoeffsTransRec[] = { m_CmpCoeffsTransRec[0], m_CmpCoeffsTransRec[1], m_CmpCoeffsTransRec[2], m_CmpCoeffsTransRec[3] };
  const int16* ConstCmpCoeffsScan    [] = { m_CmpCoeffsScan    [0], m_CmpCoeffsScan    [1], m_CmpCoeffsScan    [2], m_CmpCoeffsScan    [3] };
  const int16* ConstCmpCoeffsScanAux [] = { m_CmpCoeffsScanAux [0], m_CmpCoeffsScanAux [1], m_CmpCoeffsScanAux [2], m_CmpCoeffsScanAux [3] };
  const int16* ConstCmpCoeffsScanOpt [] = { m_CmpCoeffsScanOpt [0], m_CmpCoeffsScanOpt [1], m_CmpCoeffsScanOpt [2], m_CmpCoeffsScanOpt [3] };

  tTimePoint TP0 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  xFwdTransformPic(m_CmpCoeffsTransOrg, Picture);

  tTimePoint TP1 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  xFwdQuantScanPic(m_CmpCoeffsScan, ConstCmpCoeffsTransOrg, m_QuantMain);

  tTimePoint TP2 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  if(m_UseRDOQ)
  {
    //base point
    xInvScanQuantPic(m_CmpCoeffsTransRec, ConstCmpCoeffsScan, m_QuantMain);
    xInvTransformPic(&m_PicRec          , ConstCmpCoeffsTransRec);
    int64V4 EstNumBitsMain = xHuffEstPic (ConstCmpCoeffsScan);
    int64V4 DistortionMain = xCalcPicSSDs(Picture, &m_PicRec);

    //lower point
    int64V4 EstNumBitsAuxD = { 0,0,0,0 };
    int64V4 DistortionAuxD = { 0,0,0,0 };
    if(m_Quality > 1)
    {
      xFwdQuantScanPic(m_CmpCoeffsScanAux , ConstCmpCoeffsTransOrg, m_QuantAuxD);
      xInvScanQuantPic(m_CmpCoeffsTransRec, ConstCmpCoeffsScanAux , m_QuantAuxD);
      xInvTransformPic(&m_PicRec          , ConstCmpCoeffsTransRec);
      EstNumBitsAuxD = xHuffEstPic (ConstCmpCoeffsScanAux);
      DistortionAuxD = xCalcPicSSDs(Picture, &m_PicRec);
    }
    
    //higher point
    int64V4 EstNumBitsAuxI = { 0,0,0,0 };
    int64V4 DistortionAuxI = { 0,0,0,0 };
    if(m_Quality < 100)
    {
      xFwdQuantScanPic(m_CmpCoeffsScanAux, ConstCmpCoeffsTransOrg, m_QuantAuxI);
      xInvScanQuantPic(m_CmpCoeffsTransRec, ConstCmpCoeffsScanAux, m_QuantAuxI);
      xInvTransformPic(&m_PicRec, ConstCmpCoeffsTransRec);
      EstNumBitsAuxI = xHuffEstPic(ConstCmpCoeffsScanAux);
      DistortionAuxI = xCalcPicSSDs(Picture, &m_PicRec);
    }

    //local lambda
    int64V4 DeltaEstNumBitsD = EstNumBitsMain - EstNumBitsAuxD;
    int64V4 DeltaDistortionD = DistortionMain - DistortionAuxD;
    int64V4 DeltaEstNumBitsI = EstNumBitsMain - EstNumBitsAuxI;
    int64V4 DeltaDistortionI = DistortionMain - DistortionAuxI;

    flt64V4 LambdaD = -(flt64V4)DeltaDistortionD / (flt64V4)DeltaEstNumBitsD;
    flt64V4 LambdaI = -(flt64V4)DeltaDistortionI / (flt64V4)DeltaEstNumBitsI;
    if     (m_Quality > 1 && m_Quality < 100) { m_Lambda = (LambdaD + LambdaI) / 2.0; }
    else if(m_Quality > 1                   ) { m_Lambda = LambdaD; }
    else if(m_Quality < 100                 ) { m_Lambda = LambdaI; }

    if(m_VerboseLevel >= 5)
    {
      std::string Dump = "LambdaEstimation\n";
      Dump += fmt::format("QuantMain EstNumBits={:d} {:d} {:d}    Distortion={:d} {:d} {:d}\n", EstNumBitsMain[0], EstNumBitsMain[1], EstNumBitsMain[2], DistortionMain[0], DistortionMain[1], DistortionMain[2]);
      Dump += fmt::format("QuantAuxD EstNumBits={:d} {:d} {:d}    Distortion={:d} {:d} {:d}\n", EstNumBitsAuxD[0], EstNumBitsAuxD[1], EstNumBitsAuxD[2], DistortionAuxD[0], DistortionAuxD[1], DistortionAuxD[2]);
      Dump += fmt::format("QuantAuxI EstNumBits={:d} {:d} {:d}    Distortion={:d} {:d} {:d}\n", EstNumBitsAuxI[0], EstNumBitsAuxI[1], EstNumBitsAuxI[2], DistortionAuxI[0], DistortionAuxI[1], DistortionAuxI[2]);
      Dump += fmt::format("LambdaD = {} {} {}\n", LambdaD[0], LambdaD[1], LambdaD[2]);
      Dump += fmt::format("LambdaI = {} {} {}\n", LambdaI[0], LambdaI[1], LambdaI[2]);
      Dump += fmt::format("Lambda  = {} {} {}\n", m_Lambda[0], m_Lambda[1], m_Lambda[2]);
      fmt::print(Dump);
    }
  }

  tTimePoint TP3 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  if(m_UseRDOQ)
  {
    xOptimizePic(m_CmpCoeffsScanOpt, ConstCmpCoeffsScan, Picture);

    if(!m_OptimizeLuma)
    {
      const int32 AreaLm = m_MCUsMulWidth[(int32)eCmp::LM] * m_MCUsMulHeight[(int32)eCmp::LM];
      memcpy(m_CmpCoeffsScanOpt[(int32)eCmp::LM], m_CmpCoeffsScan[(int32)eCmp::LM], AreaLm * sizeof(int16));
    }
    if(!m_OptimizeChroma)
    {
      const int32 AreaCb = m_MCUsMulWidth[(int32)eCmp::CB] * m_MCUsMulHeight[(int32)eCmp::CB];
      memcpy(m_CmpCoeffsScanOpt[(int32)eCmp::CB], m_CmpCoeffsScan[(int32)eCmp::CB], AreaCb * sizeof(int16));
      const int32 AreaCr = m_MCUsMulWidth[(int32)eCmp::CR] * m_MCUsMulHeight[(int32)eCmp::CR];
      memcpy(m_CmpCoeffsScanOpt[(int32)eCmp::CR], m_CmpCoeffsScan[(int32)eCmp::CR], AreaCr * sizeof(int16));
    }
  }
  
  tTimePoint TP4 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  xHuffEncPic(Buffer, m_UseRDOQ ? ConstCmpCoeffsScanOpt : ConstCmpCoeffsScan);

  tTimePoint TP5 = m_GatherTimeStats ? tClock::now() : tTimePoint();
  
  m_TotalPictureIters  += 1;
  m_TotalPictureTime   += TP5 - TP0;
  m_TotalTransformTime += TP1 - TP0;
  m_TotalQuantScanTime += TP2 - TP1;
  m_TotalLambdaTime    += TP3 - TP2;
  m_TotalOptimizeTime  += TP4 - TP3;
}

int64V4 xAdvancedEncoder::xCalcPicSSDs(const xPicYUV* Tst, const xPicYUV* Ref)
{
  int64V4 SSDs = xMakeVec4<int64>(0);
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    eCmp c = (eCmp)CmpIdx;
    SSDs[CmpIdx] = xDistortion::CalcSSD(Tst->getAddr(c), Ref->getAddr(c), Tst->getStride(c), Ref->getStride(c), Tst->getWidth(c), Tst->getHeight(c));
  }
  return SSDs;
}

void xAdvancedEncoder::xFwdTransformPic(int16* CoeffsTransV[], const xPicYUV* Picture)
{
  const uint16* CmpPtrV   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrideV[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  //loop over MCUs
  for (int32 MCU_Idx = 0; MCU_Idx < m_NumMCUsInArea; MCU_Idx++)
  {
    xFwdTransformMCU(CoeffsTransV, CmpPtrV, CmpStrideV, MCU_Idx);
  }
}
void xAdvancedEncoder::xFwdTransformMCU(int16* CoeffsTransV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx)
{
  //calculate MCU position
  int32 MCU_PosV = MCU_Idx / m_NumMCUsInWidth;
  int32 MCU_PosH = MCU_Idx % m_NumMCUsInWidth;

  //org samples buffer
  uint16 SamplesOrg[c_BA];

  //transform blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 MCU_PelPosV = MCU_PosV << (2 + m_SampFactorVer[CmpIdx]);
    const int32 MCU_PelPosH = MCU_PosH << (2 + m_SampFactorHor[CmpIdx]);

    const uint16* CmpPtr    = CmpPtrV   [CmpIdx];
    const int32   CmpStride = CmpStrideV[CmpIdx];

    int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      const int32 BlockPosV = MCU_PelPosV + V * c_BS;
      const int32 BlockResV = m_CmpHeight[CmpIdx] - BlockPosV;
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {        
        const int32   BlockPosH = MCU_PelPosH + H * c_BS;
        const int32   BlockResH = m_CmpWidth[CmpIdx] - BlockPosH;
        const uint16* BlockPtr  = CmpPtr + BlockPosV * CmpStride + BlockPosH;     
        if     (BlockResV >= 8 && BlockResH >= 8) { loadEntireBlock(SamplesOrg, BlockPtr, CmpStride); } //C++20 TODO use [[likely]]
        else if(BlockResV >  0 && BlockResH >  0) { loadExtendBlock(SamplesOrg, BlockPtr, CmpStride, xMin(BlockResH, c_BS), xMin(BlockResV, c_BS)); }
        else                                      {
          zeroEntireBlock(SamplesOrg);
        }

        const int32 CoeffTransOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;
        xTransform::FwdTransformDCT_8x8(CoeffsTransV[CmpIdx] + CoeffTransOffset, SamplesOrg);
        CoeffsTransV[CmpIdx][CoeffTransOffset] -= xTransformConstants::c_FwdDcCorr; //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done be DC -= 
        BlockIdx++;
      }
    }
  }
}
void xAdvancedEncoder::xInvTransformPic(xPicYUV* Picture, const int16* CoeffsTransV[])
{
        uint16* CmpPtrs   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrides[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  //loop over MCUs
  for(int32 MCU_Idx = 0; MCU_Idx < m_NumMCUsInArea; MCU_Idx++)
  {
    xInvTransformMCU(CmpPtrs, CmpStrides, CoeffsTransV, MCU_Idx);
  }
}
void xAdvancedEncoder::xInvTransformMCU(uint16* CmpPtrV[], const int32 CmpStrideV[], const int16* CoeffsTransV[], int32 MCU_Idx)
{
  //calculate MCU position
  int32 MCU_PosV = MCU_Idx / m_NumMCUsInWidth;
  int32 MCU_PosH = MCU_Idx % m_NumMCUsInWidth;

  //org samples buffer
  int16  CoeffsTrans[c_BA];
  uint16 SamplesRec [c_BA];

  //transform blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 MCU_PelPosV = MCU_PosV << (2 + m_SampFactorVer[CmpIdx]);
    const int32 MCU_PelPosH = MCU_PosH << (2 + m_SampFactorHor[CmpIdx]);

          uint16* CmpPtr    = CmpPtrV   [CmpIdx];
    const int32   CmpStride = CmpStrideV[CmpIdx];

    int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      const int32 BlockPosV = MCU_PelPosV + V * c_BS;
      const int32 BlockResV = m_CmpHeight[CmpIdx] - BlockPosV;
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {
        const int32 BlockPosH = MCU_PelPosH + H * c_BS;
        const int32 BlockResH = m_CmpWidth[CmpIdx] - BlockPosH;
        uint16* restrict BlockPtr = CmpPtr + BlockPosV * CmpStride + BlockPosH;
        const int32 CoeffTransOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;

        memcpy(CoeffsTrans, CoeffsTransV[CmpIdx] + CoeffTransOffset, c_BA * sizeof(int16));
        CoeffsTrans[0] += xTransformConstants::c_InvDcCorr; //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done be DC -= 
        xTransform::InvTransformDCT_8x8(SamplesRec, CoeffsTrans);

        if     (BlockResV >= 8 && BlockResH >= 8) { storeEntireBlock (BlockPtr, SamplesRec, CmpStride); } //C++20 TODO use [[likely]]
        else if(BlockResV >  0 && BlockResH >  0) { storePartialBlock(BlockPtr, SamplesRec, CmpStride, xMin(BlockResH, c_BS), xMin(BlockResV, c_BS)); }
        else                                      { /* do nothing */ }

        BlockIdx++;
      }
    }
  }
}

void xAdvancedEncoder::xFwdQuantScanPic(int16* CoeffsScanV[], const int16* CoeffsTransV[], const xQuantizerSet& Quant)
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int32 QuantTabIdx = m_SOF0.getQuantTableId(eCmp(CmpIdx));
    xFwdQuantScanCmp(CoeffsScanV[CmpIdx], CoeffsTransV[CmpIdx], m_NumBlocks[CmpIdx], Quant.getQuantizer(QuantTabIdx));
  }  
}
void xAdvancedEncoder::xFwdQuantScanCmp(int16* CoeffsScan, const int16* CoeffsTrans, int32 NumBlocks, const xQuantizer& Quant)
{
  int16 CoeffsQuant[c_BA];

  for(int32 BlockIdx = 0; BlockIdx < NumBlocks; BlockIdx++)
  {
    const int32 BlockOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;
    Quant.QuantScale(CoeffsQuant, CoeffsTrans + BlockOffset);
    xScan::Scan(CoeffsScan + BlockOffset, CoeffsQuant);
  }
}
void xAdvancedEncoder::xInvScanQuantPic(int16* CoeffsTransV[], const int16* CoeffsScanV[], const xQuantizerSet& Quant)
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int32 QuantTabIdx = m_SOF0.getQuantTableId(eCmp(CmpIdx));
    xInvScanQuantCmp(CoeffsTransV[CmpIdx], CoeffsScanV[CmpIdx], m_NumBlocks[CmpIdx], Quant.getQuantizer(QuantTabIdx));
  }
}
void xAdvancedEncoder::xInvScanQuantCmp(int16* CoeffsTrans, const int16* CoeffsScan, int32 NumBlocks, const xQuantizer& Quant)
{
  int16 CoeffsQuant[c_BA];

  for(int32 BlockIdx = 0; BlockIdx < NumBlocks; BlockIdx++)
  {
    const int32 BlockOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;
    xScan::InvScan(CoeffsQuant, CoeffsScan + BlockOffset);
    Quant.InvScale(CoeffsTrans + BlockOffset, CoeffsQuant);
  }
}

int64V4 xAdvancedEncoder::xHuffEstPic(const int16* CoeffsScanV[])
{
  int64V4 EstNumBits = xMakeVec4<int64>(0);
  if(m_RestartInterval == 0) //no division - encode entire picture at once
  {
    EstNumBits += xHuffEstSlc(CoeffsScanV, 0, m_NumMCUsInArea - 1);
  }
  else //divide picture into independent slices
  {
    for(int32 SliceIdx = 0, MCU_IdxFirst = 0; MCU_IdxFirst < m_NumMCUsInArea; SliceIdx++, MCU_IdxFirst += m_RestartInterval)
    {
      int32 MCU_IdxLast = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      EstNumBits += xHuffEstSlc(CoeffsScanV, MCU_IdxFirst, MCU_IdxLast);
      //if(MCU_IdxLast != m_NumMCUsInArea - 1) { xJFIF::WriteRST(OutputBuffer, (uint8)((uint32)SliceIdx & (uint32)0x07)); }
    }
  }
  return EstNumBits;
}
int64V4 xAdvancedEncoder::xHuffEstSlc(const int16* CoeffsScanV[], int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  int64V4 EstNumBits = xMakeVec4<int64>(0);
  m_EntropyEst.StartSlice();
  //loop over MCUs
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++)
  {
    EstNumBits += xHuffEstMCU(CoeffsScanV, MCU_Idx);
  }
  return EstNumBits;
}
int64V4 xAdvancedEncoder::xHuffEstMCU(const int16* CoeffsScanV[], int32 MCU_Idx)
{
  int64V4 EstNumBits = xMakeVec4<int64>(0);

  //estimate blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int32 HuffTabIdDC = m_SOS.getHuffTableIdDC(eCmp(CmpIdx));
    int32 HuffTabIdAC = m_SOS.getHuffTableIdAC(eCmp(CmpIdx));
    int32 BlockIdx    = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {
        const int32 CoeffScanOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;        
        EstNumBits[CmpIdx] += m_EntropyEst.EstimateBlock(CoeffsScanV[CmpIdx] + CoeffScanOffset, eCmp(CmpIdx), HuffTabIdDC, HuffTabIdAC);
        BlockIdx++;
      }
    }
  }
  return EstNumBits;
}
uint64 xAdvancedEncoder::xCalcDistBLK(const int16* ScanCoeffs, const uint16* SamplesOrg, int32 QuantTabId)
{
  int16  TmpQuantCoeffs[c_BA];
  int16  TmpTransCoeffs[c_BA];
  uint16 TmpSamples    [c_BA];

  xScan::InvScan(TmpQuantCoeffs, ScanCoeffs);
  m_QuantMain.InvScale(TmpTransCoeffs, TmpQuantCoeffs, QuantTabId);
  TmpTransCoeffs[0] += xTransformConstants::c_InvDcCorr; //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done be DC -= 
  xTransform::InvTransformDCT_8x8(TmpSamples, TmpTransCoeffs);
  uint64 SSD = xDistortion::CalcSSD(SamplesOrg, TmpSamples, c_BS, c_BS, c_BS, c_BS);
  return SSD;
}

void xAdvancedEncoder::xOptimizePic(int16* OptCoeffsScanV[], const int16* CoeffsScanV[], const xPicYUV* Picture)
{
  if(m_RestartInterval == 0) //no division - encode entire picture at once
  {
    xOptimizeSlc(OptCoeffsScanV, CoeffsScanV, Picture, 0, m_NumMCUsInArea - 1);
  }
  else //divide picture into independent slices
  {
    for(int32 SliceIdx = 0, MCU_IdxFirst = 0; MCU_IdxFirst < m_NumMCUsInArea; SliceIdx++, MCU_IdxFirst += m_RestartInterval)
    {
      int32 MCU_IdxLast = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      xOptimizeSlc(OptCoeffsScanV, CoeffsScanV, Picture, MCU_IdxFirst, MCU_IdxLast);
    }
  }
}
void xAdvancedEncoder::xOptimizeSlc(int16* OptCoeffsScanV[], const int16* CoeffsScanV[], const xPicYUV* Picture, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  m_EntropyEst.StartSlice();

  const uint16* CmpPtrV   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrideV[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  //loop over MCUs
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++)
  {
    xOptimizeMCU(OptCoeffsScanV, CoeffsScanV, CmpPtrV, CmpStrideV, MCU_Idx);
  }
}
void xAdvancedEncoder::xOptimizeMCU(int16* OptCoeffsScanV[], const int16* CoeffsScanV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx)
{
  //calculate MCU position
  int32 MCU_PosV = MCU_Idx / m_NumMCUsInWidth;
  int32 MCU_PosH = MCU_Idx % m_NumMCUsInWidth;

  //org samples buffer
  uint16 SamplesOrg[c_BA];

  //transform blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    if(m_UseRDOQ && ((CmpIdx == (int32)eCmp::LM && m_OptimizeLuma) || (CmpIdx != (int32)eCmp::LM && m_OptimizeChroma)))
    {
      const int32 MCU_PelPosV = MCU_PosV << (2 + m_SampFactorVer[CmpIdx]);
      const int32 MCU_PelPosH = MCU_PosH << (2 + m_SampFactorHor[CmpIdx]);

      const uint16* CmpPtr    = CmpPtrV[CmpIdx];
      const int32   CmpStride = CmpStrideV[CmpIdx];

      int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
      for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
      {
        const int32 BlockPosV = MCU_PelPosV + V * c_BS;
        const int32 BlockResV = m_CmpHeight[CmpIdx] - BlockPosV;

        for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
        {
          const int32 BlockPosH = MCU_PelPosH + H * c_BS;
          const int32 BlockResH = m_CmpWidth[CmpIdx] - BlockPosH;

          const uint16* BlockPtr = CmpPtr + BlockPosV * CmpStride + BlockPosH;          

          if     (BlockResV >= 8 && BlockResH >= 8) { loadEntireBlock(SamplesOrg, BlockPtr, CmpStride); } //C++20 TODO use [[likely]]
          else if(BlockResV >  0 && BlockResH >  0) { loadExtendBlock(SamplesOrg, BlockPtr, CmpStride, xMin(BlockResH, c_BS), xMin(BlockResV, c_BS)); }
          else                                      { zeroEntireBlock(SamplesOrg); }

          const int32 CoeffTransOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;
          xOptimizeBLK(OptCoeffsScanV[CmpIdx] + CoeffTransOffset, CoeffsScanV[CmpIdx] + CoeffTransOffset, SamplesOrg, eCmp(CmpIdx));
          BlockIdx++;
        }
      }
    }
  }
}
void xAdvancedEncoder::xOptimizeBLK(int16* OptCoeffScan, const int16* CoeffsScan, const uint16* SamplesOrg, eCmp CmpId)
{
  const int32 QuantTabId  = m_SOF0.getQuantTableId(CmpId);
  const int32 HuffTabIdDC = m_SOS.getHuffTableIdDC(CmpId);
  const int32 HuffTabIdAC = m_SOS.getHuffTableIdAC(CmpId);  
  const flt64 Lambda      = m_Lambda[(int32)CmpId];
  const int32 LastDC      = m_EntropyEst.getLastDC(CmpId);

  int32 LastNonZero = xEntropyCommon::findLastNonZero(CoeffsScan);
  if(LastNonZero == 0) { memcpy(OptCoeffScan, CoeffsScan, c_BA * sizeof(int16)); return; } //only DC - nothing to do here

  int32  BestBits = m_EntropyEst.EstimateBlockStateless(CoeffsScan, LastDC, HuffTabIdDC, HuffTabIdAC);
  uint64 BestDist = xCalcDistBLK(CoeffsScan, SamplesOrg, QuantTabId);
  double BestCost = (double)BestDist + Lambda * (double)BestBits;

  int16 TmpCoeffsScan[c_BA];
  memcpy(TmpCoeffsScan, CoeffsScan, c_BA * sizeof(int16));

  for (int32 PassIdx = 0; PassIdx < m_NumBlockOptPasses; PassIdx++)
  {    
    for (int32 i = LastNonZero; i >= 1; i--)
    {
      const int16 OrgCoeff  = TmpCoeffsScan[i];
      if (!m_ProcessZeroCoeffs && OrgCoeff == 0) { continue; }
      
      int16 BestCoeff = TmpCoeffsScan[i];
      //try zero
      if(OrgCoeff != 0)
      {
        TmpCoeffsScan[i] = 0;
        int32  CurrBits = m_EntropyEst.EstimateBlockStateless(TmpCoeffsScan, LastDC, HuffTabIdDC, HuffTabIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsScan, SamplesOrg, QuantTabId);
        double CurrCost = (double)CurrDist + Lambda * (double)CurrBits;
        if (CurrCost < BestCost)
        {
          BestBits  = CurrBits;
          BestCost  = CurrCost;
          BestCoeff = 0;
        }
      }

      //try +1
      if(OrgCoeff != -1)
      {
        TmpCoeffsScan[i] = OrgCoeff + 1;
        int32  CurrBits = m_EntropyEst.EstimateBlockStateless(TmpCoeffsScan, LastDC, HuffTabIdDC, HuffTabIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsScan, SamplesOrg, QuantTabId);
        double CurrCost = (double)CurrDist + Lambda * (double)CurrBits;
        if (CurrCost < BestCost)
        {
          BestBits = CurrBits;
          BestCost  = CurrCost;
          BestCoeff = OrgCoeff + 1;
        }
      }

      //try -1  
      if(OrgCoeff != 1)
      {
        TmpCoeffsScan[i] = OrgCoeff - 1;
        int32  CurrBits = m_EntropyEst.EstimateBlockStateless(TmpCoeffsScan, LastDC, HuffTabIdDC, HuffTabIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsScan, SamplesOrg, QuantTabId);
        double CurrCost = (double)CurrDist + Lambda * (double)CurrBits;
        if (CurrCost < BestCost)
        {
          BestBits = CurrBits;
          BestCost  = CurrCost;
          BestCoeff = OrgCoeff - 1;
        }
      }

      TmpCoeffsScan[i] = BestCoeff;
    }
  }

  //int32 TestBits = m_EntropyEst.EstimateBlock(TmpCoeffsScan, CmpId, HuffTabIdDC, HuffTabIdAC);
  memcpy(OptCoeffScan, TmpCoeffsScan, c_BA * sizeof(int16));
}

void xAdvancedEncoder::xHuffEncPic(xByteBuffer* OutputBuffer, const int16* CoeffsScanV[])
{
  if(m_RestartInterval == 0) //no division - encode entire picture at once
  {
    xHuffEncSlc(OutputBuffer, CoeffsScanV, 0, m_NumMCUsInArea - 1);
  }
  else //divide picture into independent slices
  {
    for(int32 SliceIdx = 0, MCU_IdxFirst = 0; MCU_IdxFirst < m_NumMCUsInArea; SliceIdx++, MCU_IdxFirst += m_RestartInterval)
    {
      int32 MCU_IdxLast = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      xHuffEncSlc(OutputBuffer, CoeffsScanV, MCU_IdxFirst, MCU_IdxLast);
      if(MCU_IdxLast != m_NumMCUsInArea - 1) { xJFIF::WriteRST(OutputBuffer, (uint8)((uint32)SliceIdx & (uint32)0x07)); }
    }
  }
}
void xAdvancedEncoder::xHuffEncSlc(xByteBuffer* OutputBuffer, const int16* CoeffsScanV[], int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  tTimePoint TP0 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  m_EntropyBuffer.reset();
  m_EntropyEnc.StartSlice(&m_EntropyBuffer);

  //loop over MCUs
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++)
  {
    xHuffEncMCU(CoeffsScanV, MCU_Idx);
  }

  m_EntropyEnc.FinishSlice();

  tTimePoint TP1 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  //copy to output and add stuffing
  xJFIF::AddStuffing(OutputBuffer, &m_EntropyBuffer);

  tTimePoint TP2 = m_GatherTimeStats ? tClock::now() : tTimePoint();

  if(m_GatherTimeStats)
  {
    m_TotalSliceIters   += 1;
    m_TotalEntropyTime  += TP2 - TP0;
    m_TotalStuffingTime += TP2 - TP1;
  }

}
void xAdvancedEncoder::xHuffEncMCU(const int16* CoeffsScanV[], int32 MCU_Idx)
{
  //estimate blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int32 HuffTabIdDC = m_SOS.getHuffTableIdDC(eCmp(CmpIdx));
    int32 HuffTabIdAC = m_SOS.getHuffTableIdAC(eCmp(CmpIdx));

    int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {
        const int32 CoeffScanOffset = BlockIdx << xJPEG_Constants::c_Log2BlockArea;        
        m_EntropyEnc.EncodeBlock(CoeffsScanV[CmpIdx] + CoeffScanOffset, eCmp(CmpIdx), HuffTabIdDC, HuffTabIdAC);
        BlockIdx++;
      }
    }
  }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG