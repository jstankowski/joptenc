/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJFIF.h"

namespace PMBB_NAMESPACE::JPEG {

static const uint32 c_ParserBuffer = 4194304; //4MB = 1024 pages TODO

//=============================================================================================================================================================================
// APP0
//=============================================================================================================================================================================
int32 xJFIF::xAPP0::Absorb(xByteBuffer* Input, int32 SegmentLength)
{
  [[maybe_unused]]uint32 JFIF = xReadFourCC(Input); //Identifier: ASCII "JFIF"
  [[maybe_unused]]uint8  Null = xRead8     (Input); //Identifier: ASCII "NULL"
  m_VersionMajor              = xRead8     (Input); //Major version
  m_VersionMinor              = xRead8     (Input); //Minor version
  m_DensityUnits              = xRead8     (Input); //Density unit
  m_DensityX                  = xRead16    (Input); //Density X
  m_DensityY                  = xRead16    (Input); //Density Y
  m_ThumbnailSizeX            = xRead8     (Input); //Thumbnail size X
  m_ThumbnailSizeY            = xRead8     (Input); //Thumbnail size X

  int32 CurrentLength  = 16;
  int32 RemainingBytes = SegmentLength - CurrentLength;
  int32 ThumbnailSize  = m_ThumbnailSizeX * m_ThumbnailSizeY * 3; //RGB

  if(RemainingBytes != 0 && ThumbnailSize == RemainingBytes)
  {
    m_ThumbnailData.resize(ThumbnailSize);
    xReadVector(m_ThumbnailData, Input);    
    CurrentLength += ThumbnailSize;
  }

  return getLength();
}
int32 xJFIF::xAPP0::Emit(xByteBuffer* Output)
{
  xWriteFourCC(Output, xJFIF::c_JFIF           ); //Identifier: ASCII "JFIF"
  xWrite8     (Output, 0x00                    ); //Identifier: ASCII "NULL" 
  xWrite8     (Output, (uint8 )m_VersionMajor  ); //Major version
  xWrite8     (Output, (uint8 )m_VersionMinor  ); //Minor version
  xWrite8     (Output, (uint8 )m_DensityUnits  ); //Density unit
  xWrite16    (Output, (uint16)m_DensityX      ); //Density X
  xWrite16    (Output, (uint16)m_DensityY      ); //Density Y
  xWrite8     (Output, (uint8 )m_ThumbnailSizeX); //Thumbnail size X
  xWrite8     (Output, (uint8 )m_ThumbnailSizeY); //Thumbnail size X

  if(m_ThumbnailSizeX * m_ThumbnailSizeY > 0) { xWriteVector(Output, m_ThumbnailData); }

  return getLength();
}
void xJFIF::xAPP0::InitDefault()
{
  m_VersionMajor   = 1;
  m_VersionMinor   = 1;
  m_DensityUnits   = 0; //No units
  m_DensityX       = 1;
  m_DensityY       = 1;
  m_ThumbnailSizeX = 0;
  m_ThumbnailSizeY = 0;
  m_ThumbnailData.clear();
}
bool xJFIF::xAPP0::Validate()
{
  bool Valid = xFitsU8(m_VersionMajor) && xFitsU8(m_VersionMinor)
    && xFitsU8(m_DensityUnits) && xFitsU16(m_DensityX) && xFitsU16(m_DensityX)
    && xFitsU8(m_ThumbnailSizeX) && xFitsU8(m_ThumbnailSizeY);
  if(m_VersionMajor != 1) { Valid = false; }
  if(m_VersionMinor <  1) { Valid = false; }
  return Valid;
}

//=============================================================================================================================================================================
// DQT
//=============================================================================================================================================================================
int32 xJFIF::xQuantTable::Absorb(xByteBuffer* Input)
{
  uint8 QuantTabInfo = xRead8(Input );
  m_Precision        = (QuantTabInfo & 0xF0) >> 4;
  m_Idx              = QuantTabInfo & 0x0F;
  int32 QuantTabSize = m_Precision ? 128 : 64;
  m_Table.resize(QuantTabSize);
  xReadVector(m_Table, Input);
  return 1 + QuantTabSize;
}
int32 xJFIF::xQuantTable::Emit(xByteBuffer* Output) const
{
  xWrite8     (Output, ((m_Precision & 0x01) << 4) | (m_Idx & 0x03)); //Precision / Table idx
  xWriteVector(Output, m_Table); //Luma quant table
  return 1 + (int32)m_Table.size();
}
void xJFIF::xQuantTable::Init(uint8 Idx, eCmp Cmp, int32 Quality, eQTLa QuantTabLayout)
{
  m_Precision = 0;
  m_Idx       = Idx;
  m_Table.resize(64);
  xJPEG_Constants::GenerateQuantTable(m_Table.data(), Cmp, Quality, QuantTabLayout);
}
bool xJFIF::xQuantTable::Validate() const
{
  return true;
}
xJFIF::tStr xJFIF::xQuantTable::Format(const tStr& Prefix) const
{
  std::string Result = fmt::format("{}Table Idx={:d} Precision={:d}\n", Prefix, m_Idx, m_Precision);

  for(int32 y = 0; y < xJPEG_Constants::c_BlockSize; y++)
  {
    Result += Prefix;
    for(int32 x = 0; x < xJPEG_Constants::c_BlockSize; x++)
    {
      Result += fmt::format("{:>3d} ", m_Table[(y << xJPEG_Constants::c_Log2BlockSize) + x]);
    }
    Result += "\n";
  }
  return Result;
}

//=============================================================================================================================================================================
// SOF0
//=============================================================================================================================================================================
int32 xJFIF::xSOF0::Absorb(xByteBuffer* Input)
{
  m_BitDepth      = xRead8 (Input); //Bits per sample
  m_Height        = xRead16(Input); //Height
  m_Width         = xRead16(Input); //Width
  m_NumComponents = xRead8 (Input); //Num components
  for(int32 CmpIdx = 0; CmpIdx < m_NumComponents; CmpIdx++)
  {
    m_CmpId         [CmpIdx] = (eCmp)xRead8(Input);
    m_SamplingFactor[CmpIdx] = xRead8(Input);
    m_QuantTableId  [CmpIdx] = xRead8(Input);
  }
  return getLength();
}
int32 xJFIF::xSOF0::Emit(xByteBuffer* Output) const
{
  xWrite8     (Output, (uint8 )m_BitDepth     ); //Bits per sample
  xWrite16    (Output, (uint16)m_Height       ); //Height
  xWrite16    (Output, (uint16)m_Width        ); //Width
  xWrite8     (Output, (uint8 )m_NumComponents); //Num components

  for(int32 CmpIdx = 0; CmpIdx < m_NumComponents; CmpIdx++)
  {
    xWrite8(Output, (uint8)m_CmpId         [CmpIdx]);
    xWrite8(Output, (uint8)m_SamplingFactor[CmpIdx]);
    xWrite8(Output, (uint8)m_QuantTableId  [CmpIdx]);
  }
  return getLength();
}
eCrF xJFIF::xSOF0::DetermineChromaFormat() const
{
  eCrF ChromaFormat = eCrF::INVALID;
  if(m_NumComponents == 1) { ChromaFormat = eCrF::CF400; }
  else if(GetSamplingFactorH(1)==1 && GetSamplingFactorV(1)==1 && GetSamplingFactorH(2)==1 && GetSamplingFactorV(2)==1)
  {
    if     (GetSamplingFactorH(0) == 1 && GetSamplingFactorV(0)==1) { ChromaFormat = eCrF::CF444; }
    else if(GetSamplingFactorH(0) == 2 && GetSamplingFactorV(0)==1) { ChromaFormat = eCrF::CF422; }
    else if(GetSamplingFactorH(0) == 2 && GetSamplingFactorV(0)==2) { ChromaFormat = eCrF::CF420; }
  }
  return ChromaFormat;
}
void xJFIF::xSOF0::Init(int32 Height, int32 Width, int32 BitDepth, eCrF ChromaFormat, int32 NumQuantTables)
{
  m_Height        = Height;
  m_Width         = Width;
  m_BitDepth      = BitDepth;
  m_NumComponents = ChromaFormat != eCrF::CF400 ? 3 : 1;
  switch(ChromaFormat)
  {
    case eCrF::CF444: m_CmpId[(int32)eCmp::LM] = eCmp::LM; m_SamplingFactor[(int32)eCmp::LM] = ((1 << 4) + (1)); m_QuantTableId[(int32)eCmp::LM] = 0; break; //Lm - cmp idx, sampling factor H / sampling factor V, quant table idx
    case eCrF::CF422: m_CmpId[(int32)eCmp::LM] = eCmp::LM; m_SamplingFactor[(int32)eCmp::LM] = ((2 << 4) + (1)); m_QuantTableId[(int32)eCmp::LM] = 0; break; //Lm - cmp idx, sampling factor H / sampling factor V, quant table idx
    case eCrF::CF420: m_CmpId[(int32)eCmp::LM] = eCmp::LM; m_SamplingFactor[(int32)eCmp::LM] = ((2 << 4) + (2)); m_QuantTableId[(int32)eCmp::LM] = 0; break; //Lm - cmp idx, sampling factor H / sampling factor V, quant table idx
    case eCrF::CF400: m_CmpId[(int32)eCmp::LM] = eCmp::LM; m_SamplingFactor[(int32)eCmp::LM] = ((1 << 4) + (1)); m_QuantTableId[(int32)eCmp::LM] = 0; break; //Lm - cmp idx, sampling factor H / sampling factor V, quant table idx
    default: assert(0); break;
  }
  if(ChromaFormat != eCrF::CF400)
  {    
    m_CmpId[(int32)eCmp::CB] = eCmp::CB; m_SamplingFactor[(int32)eCmp::CB] = ((1 << 4) + (1)); m_QuantTableId[(int32)eCmp::CB] = (int8)xMin(1, NumQuantTables - 1); //Cb - cmp idx, sampling factor H / sampling factor V, quant table idx
    m_CmpId[(int32)eCmp::CR] = eCmp::CR; m_SamplingFactor[(int32)eCmp::CR] = ((1 << 4) + (1)); m_QuantTableId[(int32)eCmp::CR] = (int8)xMin(2, NumQuantTables - 1); //Cr - cmp idx, sampling factor H / sampling factor V, quant table idx
  }
}
bool xJFIF::xSOF0::Validate() const
{
  return true;
}

//=============================================================================================================================================================================
// DHT
//=============================================================================================================================================================================
int32 xJFIF::xHuffTable::Absorb(xByteBuffer* Input)
{
  uint8 HuffTabInfo = xRead8(Input );
  m_Class = (eHuffClass)((HuffTabInfo & 0xF0) >> 4);
  m_Idx   = HuffTabInfo & 0x0F;
  xReadMemmory(m_CodeLengths.data(), 16, Input);
  int32 NumSymbols = 0;
  for(int32 i=0; i < 16; i++) { NumSymbols += m_CodeLengths[i]; }
  m_CodeSymbols.resize(NumSymbols);
  xReadVector(m_CodeSymbols, Input );
  return 1 + 16 + NumSymbols;
}
int32 xJFIF::xHuffTable::Emit(xByteBuffer* Output) const
{
  xWrite8      (Output, ((uint8)m_Class<<4) + m_Idx);
  xWriteMemmory(Output, m_CodeLengths.data(), 16);
  xWriteVector (Output, m_CodeSymbols);
  return getLength();
}
void xJFIF::xHuffTable::InitDefault(uint8 Idx, eHuffClass Class, eCmp Cmp)
{
  m_Class = Class;
  m_Idx   = Idx;

  if(Cmp == eCmp::LM)
  {
    if(Class == eHuffClass::DC)
    {
      memcpy(m_CodeLengths.data(), xJPEG_Constants::m_CodeLengthLumaDC, sizeof(xJPEG_Constants::m_CodeLengthLumaDC));
      m_CodeSymbols.assign((const byte*)xJPEG_Constants::m_CodeSymbolLumaDC, (const byte*)xJPEG_Constants::m_CodeSymbolLumaDC+sizeof(xJPEG_Constants::m_CodeSymbolLumaDC));
    }
    else if(Class == eHuffClass::AC)
    {
      memcpy(m_CodeLengths.data(), xJPEG_Constants::m_CodeLengthLumaAC, sizeof(xJPEG_Constants::m_CodeLengthLumaAC));
      m_CodeSymbols.assign((const byte*)xJPEG_Constants::m_CodeSymbolLumaAC, (const byte*)xJPEG_Constants::m_CodeSymbolLumaAC+sizeof(xJPEG_Constants::m_CodeSymbolLumaAC));
    }
  }
  else
  {
    if(Class == eHuffClass::DC)
    {
      memcpy(m_CodeLengths.data(), xJPEG_Constants::m_CodeLengthChromaDC, sizeof(xJPEG_Constants::m_CodeLengthChromaDC));
      m_CodeSymbols.assign((const byte*)xJPEG_Constants::m_CodeSymbolChromaDC, (const byte*)xJPEG_Constants::m_CodeSymbolChromaDC+sizeof(xJPEG_Constants::m_CodeSymbolChromaDC));
    }
    else if(Class == eHuffClass::AC)
    {
      memcpy(m_CodeLengths.data(), xJPEG_Constants::m_CodeLengthChromaAC, sizeof(xJPEG_Constants::m_CodeLengthChromaAC));
      m_CodeSymbols.assign((const byte*)xJPEG_Constants::m_CodeSymbolChromaAC, (const byte*)xJPEG_Constants::m_CodeSymbolChromaAC+sizeof(xJPEG_Constants::m_CodeSymbolChromaAC));
    }
  }
}
bool xJFIF::xHuffTable::Validate() const
{
  return true;
}

//=============================================================================================================================================================================
// SOS
//=============================================================================================================================================================================
int32 xJFIF::xSOS::Absorb(xByteBuffer* Input)
{
  m_NumComponents  = xRead8 (Input);
  for(int32 CmpIdx = 0; CmpIdx < m_NumComponents; CmpIdx++)
  {
    m_CmpId      [CmpIdx] = (eCmp)xRead8(Input);
    m_HuffTableId[CmpIdx] = xRead8(Input);
  }
  m_SpectralSelectionStart  = xRead8(Input);
  m_SpectralSelectionEnd    = xRead8(Input);
  m_SuccessiveApproximation = xRead8(Input);
  return 4 + m_NumComponents * 2;
}
int32 xJFIF::xSOS::Emit(xByteBuffer* Output) const
{
  xWrite8 (Output, (uint8)m_NumComponents); //num components
  for(int32 CmpIdx = 0; CmpIdx < m_NumComponents; CmpIdx++)
  {
    xWrite8(Output, (uint8)m_CmpId[CmpIdx]);
    xWrite8(Output, (uint8)m_HuffTableId [CmpIdx]);
  }
  xWrite8(Output, (uint8)m_SpectralSelectionStart );
  xWrite8(Output, (uint8)m_SpectralSelectionEnd   );
  xWrite8(Output, (uint8)m_SuccessiveApproximation);
  return 4 + m_NumComponents * 2;
}
void xJFIF::xSOS::Init(int32 NumComponents, int32 LumaHuffTabIdx, int32 ChromaHuffTabIdx)
{
  m_NumComponents = NumComponents;
  for(int32 CmpIdx = 0; CmpIdx < m_NumComponents; CmpIdx++)
  {
    m_CmpId[CmpIdx] = (eCmp)(CmpIdx);
    if(CmpIdx == 0) { m_HuffTableId[CmpIdx] = (((LumaHuffTabIdx   & 0x03) << 4) + (LumaHuffTabIdx   & 0x03)); } //Huffman table to use: DC table/AC table
    else            { m_HuffTableId[CmpIdx] = (((ChromaHuffTabIdx & 0x03) << 4) + (ChromaHuffTabIdx & 0x03)); } //Huffman table to use: DC table/AC table
  }
  m_SpectralSelectionStart  = 0;
  m_SpectralSelectionEnd    = 63;
  m_SuccessiveApproximation = 0;
}
bool  xJFIF::xSOS::Validate() const
{
  return true;
}

//=============================================================================================================================================================================
// JFIF
//=============================================================================================================================================================================
bool xJFIF::ReadAPP0(xByteBuffer* Input, xAPP0& APP0)
{
  if(xPeekMarker(Input) != eMarker::APP0) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  int32                   SegmentLength = xRead16    (Input); //Length

  APP0.Absorb(Input, SegmentLength);
  return APP0.Validate();
}
void xJFIF::WriteAPP0(xByteBuffer* Output, xAPP0& APP0)
{ 
  xWriteMarker(Output, eMarker::APP0           ); //Marker
  xWrite16    (Output, (uint16)APP0.getLength()); //Length
  APP0.Emit(Output);
}
void xJFIF::WriteDefaultAPP0(xByteBuffer* Output)
{ 
  xAPP0 APP0;
  APP0.InitDefault();
  WriteAPP0(Output, APP0);
}
bool xJFIF::ReadDQT(xByteBuffer* Input , std::vector<xQuantTable>& QuantTables)
{
  if(xPeekMarker(Input) != eMarker::DQT) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  int32                   SegmentLength = xRead16    (Input); //Length

  int32 RemainingBytes  = SegmentLength - 2;
  while(RemainingBytes > 0)
  {
    xQuantTable QuantTable;
    int32 NumBytes = QuantTable.Absorb(Input);
    QuantTables.push_back(std::move(QuantTable));
    RemainingBytes -= NumBytes;
  }
  return true;
}
void xJFIF::WriteDQT(xByteBuffer* Output, const std::vector<xQuantTable>& QuantTables)
{
  //LenField=2, QT idx=1, QT=64 or 128
  int32 SegmentLength = 2;
  for(const xQuantTable& QuantTable : QuantTables) { SegmentLength += QuantTable.getLength(); }

  xWriteMarker(Output, eMarker::DQT         ); //marker FFDB
  xWrite16    (Output, (uint16)SegmentLength); //length (2bytes)
  for(const xQuantTable& QuantTable : QuantTables) { QuantTable.Emit(Output); }
}
bool xJFIF::ReadDRI(xByteBuffer* Input , int32& RestartInterval)
{
  if(xPeekMarker(Input) != eMarker::DRI) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  int32                   SegmentLength = xRead16    (Input); //Length
  if(SegmentLength != 4) { return false; }

  RestartInterval = xRead16(Input ); //restart interval (2bytes) 
  return true;
}
void xJFIF::WriteDRI(xByteBuffer* Output, int32 RestartInterval)
{
  assert(xFitsU16(RestartInterval));
  if(RestartInterval==0) return;
  xWriteMarker(Output, eMarker::DRI           ); //marker FFDB
  xWrite16    (Output, 4                      ); //length (2bytes)
  xWrite16    (Output, (uint16)RestartInterval); //restart interval (2bytes) 
}
bool xJFIF::ReadSOF0 (xByteBuffer* Input , xSOF0& SOF0)
{
  if(xPeekMarker(Input) != eMarker::SOF0) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  [[maybe_unused]]int32   SegmentLength = xRead16    (Input); //Length

  SOF0.Absorb(Input);
  return SOF0.Validate();
}
void xJFIF::WriteSOF0(xByteBuffer* Output, xSOF0& SOF0) 
{ 
  xWriteMarker(Output, eMarker::SOF0           ); //Marker - 0=baseline
  xWrite16    (Output, (uint16)SOF0.getLength()); //Length
  SOF0.Emit(Output);
}
void xJFIF::WriteSOF0(xByteBuffer* Output, int32 Height, int32 Width, int32 BitDepth, eCrF ChromaFormat, int32 NumQuantTables)
{ 
  xSOF0 SOF0;
  SOF0.Init(Height, Width, BitDepth, ChromaFormat, NumQuantTables);
  WriteSOF0(Output, SOF0);
}
bool xJFIF::ReadDHT(xByteBuffer* Input , std::vector<xHuffTable>& HuffTables)
{
  if(xPeekMarker(Input) != eMarker::DHT) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  int32                   SegmentLength = xRead16    (Input); //Length

  int32 RemainingBytes = SegmentLength - 2;
  while(RemainingBytes > 0)
  {
    xHuffTable HuffTable;
    int32 NumBytes = HuffTable.Absorb(Input);
    HuffTables.push_back(std::move(HuffTable));
    RemainingBytes -= NumBytes;
  }
  return true;
}
void xJFIF::WriteDHT(xByteBuffer* Output, std::vector<xHuffTable>& HuffTables)
{
  int32 SegmentLength = 2;
  for(xHuffTable& HuffTable : HuffTables) { SegmentLength += HuffTable.getLength(); }

  xWriteMarker(Output, eMarker::DHT         );
  xWrite16    (Output, (uint16)SegmentLength); //length
  for(xHuffTable& HuffTable : HuffTables) { HuffTable.Emit(Output); }
}
void xJFIF::WriteDefaultDHT(xByteBuffer* Output)
{
  uint16 SegmentLength = 2 + 4 * (1 + 16) + sizeof(xJPEG_Constants::m_CodeSymbolLumaDC) + sizeof(xJPEG_Constants::m_CodeSymbolLumaAC) + sizeof(xJPEG_Constants::m_CodeSymbolChromaDC) + sizeof(xJPEG_Constants::m_CodeSymbolChromaAC);

  xWriteMarker(Output, eMarker::DHT);
  xWrite16(Output, SegmentLength); //length

  xWrite8(Output, (0<<4) + 0);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeLengthLumaDC, 16);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeSymbolLumaDC, sizeof(xJPEG_Constants::m_CodeSymbolLumaDC));

  xWrite8(Output, (1<<4) + 0);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeLengthLumaAC, 16);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeSymbolLumaAC, sizeof(xJPEG_Constants::m_CodeSymbolLumaAC));

  xWrite8(Output, (0<<4) + 1);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeLengthChromaDC, 16);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeSymbolChromaDC, sizeof(xJPEG_Constants::m_CodeSymbolChromaDC));

  xWrite8(Output, (1<<4) + 1);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeLengthChromaAC, 16);
  xWriteMemmory(Output, xJPEG_Constants::m_CodeSymbolChromaAC, sizeof(xJPEG_Constants::m_CodeSymbolChromaAC));
}
bool xJFIF::ReadSOS(xByteBuffer* Input, xSOS& SOS)
{
  if(xPeekMarker(Input) != eMarker::SOS) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  [[maybe_unused]]int32   SegmentLength = xRead16    (Input); //Length

  SOS.Absorb(Input);
  return SOS.Validate();
}
bool xJFIF::SkipSOS(xByteBuffer* Input)
{
  if(xPeekMarker(Input) != eMarker::SOS) { return false; }

  [[maybe_unused]]eMarker Marker        = xReadMarker(Input); //Marker
  int32                   SegmentLength = xRead16(Input); //Length
  xSkip(Input, SegmentLength - 2);
  return true;
}
void xJFIF::WriteSOS(xByteBuffer* Output, xSOS& SOS)
{
  xWriteMarker(Output, eMarker::SOS           ); //Marker
  xWrite16    (Output, (uint16)SOS.getLength()); //Length
  SOS.Emit(Output);
}
void xJFIF::WriteSOS(xByteBuffer* Output, int32 NumComponents, int32 LumaHuffTabIdx, int32 ChromaHuffTabIdx)
{
  xSOS SOS;
  SOS.Init(NumComponents, LumaHuffTabIdx, ChromaHuffTabIdx);
  WriteSOS(Output, SOS);
}
int8 xJFIF::ReadRST(xByteBuffer* Input)
{
  eMarker Marker = xReadMarker(Input);
  if(Marker != eMarker::ERR) { return (uint8)Marker & 0x07; }
  else                       { return NOT_VALID;            }
}
void xJFIF::WriteRST(xByteBuffer* Output, uint8 RestartIdx)
{
  RestartIdx = RestartIdx & 0x07;
  eMarker Marker = (eMarker)((uint8)eMarker::RST0 | RestartIdx);
  xWriteMarker(Output, Marker);
}
int32 xJFIF::FindSegment(xByteBuffer* Input, eMarker Marker)
{
  byte* Ptr = Input->getReadPtr();
  byte* End = Ptr + Input->getDataSize();

  if(Marker != eMarker::ERR) //selected marker
  {
    while(Ptr != End)
    {
      if(*Ptr == 0xFF)
      {
        if(((End - Ptr) > 1) && *(Ptr + 1) == (byte)Marker)
        {
          int32 Pos = (int32)(Ptr - Input->getReadPtr());
          return Pos;
        }
      }
      Ptr++;
    }
  }
  else //any marker
  {
    while(Ptr != End)
    {
      if(*Ptr == 0xFF)
      {
        if(((End - Ptr) > 1) && *(Ptr + 1) > 0)
        {
          int32 Pos = (int32)(Ptr - Input->getReadPtr());
          return Pos;
        }
      }
      Ptr++;
    }
  }
  return NOT_VALID;
}
int64 xJFIF::SeekNextSegment(std::ifstream* Input, eMarker Marker)
{
  static const int64 BufferSize = c_ParserBuffer; 

  xByteBuffer SearchBuffer(BufferSize);
  while(!Input->eof())
  {
    int64 Readed = SearchBuffer.read(Input);
    int64 Result = FindSegment(&SearchBuffer, Marker);
    if(Result >= 0)
    {
      Input->seekg(Result - Readed, std::ios_base::cur);
      return Result;
    }
    if(Readed < BufferSize) { break; }
  }

  //if(Marker != eMarker::ERR) //selected marker
  //{
  //  while(!Input->eof())
  //  {
  //    if(Input->get() == 0xFF)
  //    {
  //      if(!Input->eof() && Input->peek() == (byte)Marker)
  //      {
  //        Input->seekg(-1, Input->cur);
  //        return Input->tellg();
  //      }
  //    }
  //  }
  //}
  //else //any marker
  //{
  //  while(!Input->eof())
  //  {
  //    if(Input->get() == 0xFF)
  //    {
  //      if(!Input->eof() && Input->peek() > 0)
  //      {
  //        Input->seekg(-1, Input->cur);
  //        return Input->tellg();
  //      }
  //    }
  //  }
  //}
  return NOT_VALID;
}
void xJFIF::AddStuffing(xByteBuffer* Output, xByteBuffer* Input)
{
  byte* Src     = Input->getReadPtr();
  byte* LastSrc = Src + Input->getDataSize();
  byte* Dst     = Output->getWritePtr();

  while(Src<LastSrc)
  {    
    *(Dst++) = *(Src++);   
    if(*(Src - 1) == 0xFF)
    {
      *(Dst++) = 0x00;
    }
  }  

  int32 OutputLength = (int32)(Dst - Output->getWritePtr());
  Output->modifyWritten(OutputLength);
}
void xJFIF::RemoveStuffing(xByteBuffer* Output, xByteBuffer* Input)
{
  byte* Src     = Input->getReadPtr();
  byte* LastSrc = Src + Input->getDataSize();
  byte* Dst     = Output->getWritePtr();

  while(Src<LastSrc)
  {    
    *(Dst++) = *(Src++);   
    if(*(Src - 1) == 0xFF)
    {
      if(*Src == 0x00) { Src++; } //stuffing
      else             { Src--; Dst--; break; } //marker
    }
  }

  int32 InputLength  = (int32)(Src - Input->getReadPtr());
  Input->modifyRead(InputLength);
  int32 OutputLength = (int32)(Dst - Output->getWritePtr());
  Output->modifyWritten(OutputLength);
}

//=============================================================================================================================================================================

} //end of namespace PMBB_NAMESPACE::JPEG
