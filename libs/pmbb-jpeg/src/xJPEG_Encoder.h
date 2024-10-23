/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_CodecCommon.h"
#include "xJFIF.h"
#include "xPicYUV.h"
#include "xJPEG_Quant.h"
#include "xJPEG_Entropy.h"
#include <array>

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xAdvancedEncoder : public xCodecImplCommon
{
public:
  using tDistBits = std::tuple<int64V4, int64V4>;

protected:
  int32 m_Quality;

  //encoder behaviour
  bool    m_EmitAPP0      = true;
  bool    m_EmitQuantTabs = true;
  bool    m_EmitHuffTabs  = true;
  //RDOQ
  bool    m_UseRDOQ           = false;
  bool    m_OptimizeLuma      = false;
  bool    m_OptimizeChroma    = false;  
  bool    m_ProcessZeroCoeffs = false;
  int32   m_NumBlockOptPasses = 0;
  flt64V4 m_Lambda            = { 1.0, 1.0, 1.0, 1.0 };

  //Tools
  xQuantizerSet     m_QuantMain;
  xQuantizerSet     m_QuantAuxD;
  xQuantizerSet     m_QuantAuxI;
  xEntropyEncoder   m_EntropyEnc;
  xEntropyEstimator m_EntropyEst;
 
  //Buffers
  xPicYUV* m_PicYCbCr444 = nullptr;
  xPicYUV* m_PicYCbCr4XX = nullptr;

  int16*  m_CmpCoeffsTransOrg[c_NC];
  int16*  m_CmpCoeffsTransRec[c_NC];
  int16*  m_CmpCoeffsScan    [c_NC];
  int16*  m_CmpCoeffsScanAux [c_NC];
  int16*  m_CmpCoeffsScanOpt [c_NC];
  xPicYUV m_PicRec;

  xByteBuffer m_EntropyBuffer;

  //Profiling
  tDuration  m_TotalTransformTime = (tDuration)0;
  tDuration  m_TotalQuantScanTime = (tDuration)0;
  tDuration  m_TotalLambdaTime    = (tDuration)0;
  tDuration  m_TotalOptimizeTime  = (tDuration)0;
  tDuration  m_TotalEntropyTime   = (tDuration)0;
  tDuration  m_TotalStuffingTime  = (tDuration)0;

public:
  void   create (int32V2 PictureSize, eCrF ChromaFormat);
  void   destroy();

  void   initBaseMarkers();
  void   initQuant      (int32 Quality, eQTLa QuantTabLayout);
  void   initEntropy    (int32 RestartInterval);
  void   setMarkerEmit  (bool EmitAPP0, bool EmitQuantTabs, bool EmitHuffmanTabs);
  void   setRDOQ        (bool OptimizeLuma, bool OptimizeChroma, bool ProcessZeroCoeffs, int32 NumBlockOptPasses);
  
  void   encode(const xPicYUV* InputPicture, xByteBuffer* OutputBuffer);

  tDistBits calcDistBits(const xPicYUV* Picture);

  std::string formatAndResetStats(const std::string Prefix);

protected:
  void    xEncodePicture  (xByteBuffer* Buffer, const xPicYUV* Picture);
  int64V4 xCalcPicSSDs    (const xPicYUV* Tst, const xPicYUV* Ref);

  void xFwdTransformPic(int16* CoeffsTransV[], const xPicYUV* Picture);
  void xFwdTransformMCU(int16* CoeffsTransV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx);
  void xInvTransformPic(xPicYUV* Picture, const int16* CoeffsTransV[]);
  void xInvTransformMCU(uint16* CmpPtrV[], const int32 CmpStrideV[], const int16* CoeffsTransV[], int32 MCU_Idx);

  void        xFwdQuantScanPic(int16* CoeffScanV [], const int16* CoeffTransV[], const xQuantizerSet& Quant);
  static void xFwdQuantScanCmp(int16* CoeffScan    , const int16* CoeffTrans   , int32 NumBlocks, const xQuantizer& Quant);
  void        xInvScanQuantPic(int16* CoeffTransV[], const int16* CoeffScanV[] , const xQuantizerSet& Quant);
  static void xInvScanQuantCmp(int16* CoeffTrans   , const int16* CoeffScan    , int32 NumBlocks, const xQuantizer& Quant);

  int64V4 xHuffEstPic(const int16* CoeffsScanV[]);
  int64V4 xHuffEstSlc(const int16* CoeffsScanV[], int32 MCU_IdxFirst, int32 MCU_IdxLast);
  int64V4 xHuffEstMCU(const int16* CoeffsScanV[], int32 MCU_Idx);

  void   xOptimizePic(int16* OptCoeffsScanV[], const int16* CoeffsScanV[], const xPicYUV* Picture);
  void   xOptimizeSlc(int16* OptCoeffsScanV[], const int16* CoeffsScanV[], const xPicYUV* Picture, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void   xOptimizeMCU(int16* OptCoeffsScanV[], const int16* CoeffsScanV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx);
  void   xOptimizeBLK(int16* OptCoeffScan, const int16* CoeffsScan, const uint16* SamplesOrg, eCmp CmpId);
  uint64 xCalcDistBLK(const int16* ScanCoeffs, const uint16* SamplesOrg, int32 QuantTabId);

  void xHuffEncPic(xByteBuffer* OutputBuffer, const int16* CoeffsScanV[]);
  void xHuffEncSlc(xByteBuffer* OutputBuffer, const int16* CoeffsScanV[], int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void xHuffEncMCU(const int16* CoeffsScanV[], int32 MCU_Idx);
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG