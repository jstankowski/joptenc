/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJFIF.h"
#include "xBitstream.h"

#define X_PMBB_JPEG_MULTI_LEVEL_LOOKAHEAD 0

namespace PMBB_NAMESPACE::JPEG {

 //=====================================================================================================================================================================================

class xHuffCommon
{
protected:
  static bool xInitEncoderTables(uint32* HuffCode, uint8* HuffLen, const xJFIF::xHuffTable& HuffTable);

  static int32 xFillTmpLengths(uint8 * Lenghts, const xJFIF::xHuffTable::tCodeL& TabCodeLengths);
  static int32 xFillTmpCodes  (uint32* Codes  , const uint8* Lenghts, int32 NumLengths);
};

//=====================================================================================================================================================================================

class xHuffDecoder : public xHuffCommon
{
protected:
#if X_PMBB_JPEG_MULTI_LEVEL_LOOKAHEAD
  static const int32 c_LookAhead1st =  8; //fixed size
  static const int32 c_LookAhead2nd = 16; //fixed size
#else
  static const int32 c_LookAhead    = 10; //can be up to 16
#endif

  uint8  m_CodeSymbols[256];
  int32  m_MaxCode    [18 ]; //largest code of length k (-1 if none)
  int32  m_ValOffset  [18 ]; //huffval[] offset for codes of length k
#if X_PMBB_JPEG_MULTI_LEVEL_LOOKAHEAD
  uint16 m_Lookup1st  [1<<c_LookAhead1st];
  uint32 m_Lookup2nd  [1<<c_LookAhead2nd];
#else
  uint32  m_Lookup    [1<<c_LookAhead   ];
#endif

public:
  bool init(const xJFIF::xHuffTable& HuffTable)
  {     
    //return xCreateDerrivedDecoder(m_CodeSymbols, m_MaxCode, m_ValOffset, m_Lookup, HuffTable);
    return xInitTables(HuffTable);
  }
#if X_PMBB_JPEG_MULTI_LEVEL_LOOKAHEAD
  int32 readPrefix(xBitstreamReader* Bitstream)
  {
    uint32 Peek1st    = Bitstream->peekBits(c_LookAhead1st);
    uint32 Lockup1st  = m_Lookup1st[Peek1st];
    uint32 NumBits1st = Lockup1st >> c_LookAhead1st;

    if(NumBits1st <= c_LookAhead1st)
    {
      Bitstream->skipBits(NumBits1st);
      return Lockup1st & ((1 << c_LookAhead1st) - 1);
    }
    else
    {
      uint32 Peek2nd    = Bitstream->peekBits(c_LookAhead2nd);
      uint32 Lockup2nd  = m_Lookup2nd[Peek2nd];
      uint32 NumBits2nd = Lockup2nd >> c_LookAhead2nd;
      if(NumBits2nd <= c_LookAhead2nd)
      {
        Bitstream->skipBits(NumBits2nd);
        return Lockup2nd & ((1 << c_LookAhead2nd) - 1);
      }
      else
      {
        int32 S = Bitstream->readBits(NumBits2nd);
        while(S > m_MaxCode[NumBits2nd])
        {
          S <<= 1;
          S |= Bitstream->readBit();
          NumBits2nd++;
        }
        S = m_CodeSymbols[(S + m_ValOffset[NumBits2nd]) & 0xFF];
        return S;
      }
    }
  }
#else
  int32 readPrefix(xBitstreamReader* Bitstream)
  {
    uint32 Peek    = Bitstream->peekBits(c_LookAhead);
    uint32 Lockup  = m_Lookup[Peek];
    uint32 NumBits = Lockup >> c_LookAhead;

    if(NumBits <= c_LookAhead)
    {
      Bitstream->skipBits(NumBits);
      return Lockup & ((1 << c_LookAhead) - 1);
    }
    else
    {
      int32 S = Bitstream->readBits(NumBits);
      while(S > m_MaxCode[NumBits])
      {
        S <<= 1;
        S |= Bitstream->readBit();
        NumBits++;
      }
      S = m_CodeSymbols[(S + m_ValOffset[NumBits]) & 0xFF];
      return S;
    }    
  }
#endif
  int32 readSufix(xBitstreamReader* Bitstream, int32 NumBits)
  {
    int32 R = Bitstream->readBits(NumBits);
    int32 Value = R + (((R - (1 << (NumBits - 1))) >> 31) & ((((uint32)-1) << NumBits) + 1));
    return Value;
  }
  int32 readDC(xBitstreamReader* Bitstream)
  {
    int32 DC = readPrefix(Bitstream);
    if(DC) { DC = readSufix(Bitstream, DC); }
    return DC;
  }

protected:
  bool xInitTables(const xJFIF::xHuffTable& HuffTable);

};

//=====================================================================================================================================================================================

class xHuffEncoderDC : public xHuffCommon
{
protected:
  uint32 m_HuffCode[xJPEG_Constants::c_MaxNumCodeSymbolsDC];
  uint8  m_HuffLen [xJPEG_Constants::c_MaxNumCodeSymbolsDC];

public:
  bool init    (const xJFIF::xHuffTable& HuffTable) { if(HuffTable.getClass() != xJFIF::xHuffTable::eHuffClass::DC) { return false; } return xInitEncoderTables(m_HuffCode, m_HuffLen, HuffTable); }
  void writeDC (xBitstreamWriter* Bitstream, int32 NumBits, uint32 Remainder) { Bitstream->writeBits(m_HuffCode[NumBits], m_HuffLen[NumBits]); Bitstream->writeBits(Remainder, NumBits); }
};

class xHuffEncoderAC : public xHuffCommon
{
protected:
  uint32 m_HuffCode[xJPEG_Constants::c_MaxNumCodeSymbolsAC];
  uint8  m_HuffLen [xJPEG_Constants::c_MaxNumCodeSymbolsAC];

public:
  bool init    (xJFIF::xHuffTable& HuffTable) { if(HuffTable.getClass() != xJFIF::xHuffTable::eHuffClass::AC) { return false; } return xInitEncoderTables(m_HuffCode, m_HuffLen, HuffTable); }
  void writeAC (xBitstreamWriter* Bitstream, int32 Code, int32 NumBits, uint32 Remainder) { Bitstream->writeBits(m_HuffCode[Code], m_HuffLen[Code]); Bitstream->writeBits(Remainder, NumBits); }
  void writeZRL(xBitstreamWriter* Bitstream) { Bitstream->writeBits(m_HuffCode[0xF0], m_HuffLen[0xF0]); }
  void writeEOB(xBitstreamWriter* Bitstream) { Bitstream->writeBits(m_HuffCode[0x00], m_HuffLen[0x00]); }
};

//=====================================================================================================================================================================================

class xHuffEstimatorDC: public xHuffCommon
{
protected:
  uint8 m_HuffLen [xJPEG_Constants::c_MaxNumCodeSymbolsDC];

public:
  bool init(const xJFIF::xHuffTable& HuffTable)
  { 
    uint32 HuffCode[xJPEG_Constants::c_MaxNumCodeSymbolsDC];
    if(HuffTable.getClass() != xJFIF::xHuffTable::eHuffClass::DC) { return false; }
    return xInitEncoderTables(HuffCode, m_HuffLen, HuffTable);
  }

  int32 calcDC(int32 NumBits) const { return m_HuffLen[NumBits] + NumBits; }
};

class xHuffEstimatorAC : public xHuffCommon
{
protected:
  uint8  m_HuffLen [xJPEG_Constants::c_MaxNumCodeSymbolsAC];

public:
  bool init(const xJFIF::xHuffTable& HuffTable)
  { 
    uint32 HuffCode[xJPEG_Constants::c_MaxNumCodeSymbolsAC];
    if(HuffTable.getClass() != xJFIF::xHuffTable::eHuffClass::AC) { return false; }
    return xInitEncoderTables(HuffCode, m_HuffLen, HuffTable);
  }

  int32 calcAC (int32 Code, int32 NumBits) const { return m_HuffLen[Code] + NumBits; }
  int32 calcZRL() const { return m_HuffLen[0xF0]; }
  int32 calcEOB() const { return m_HuffLen[0x00]; }
};

//=====================================================================================================================================================================================

class xHuffCounterDC
{
protected:
  uint32 m_SymbolCount[xJPEG_Constants::c_MaxNumCodeSymbolsDC];

public:
  bool init   (          ) { memset(m_SymbolCount, 0, xJPEG_Constants::c_MaxNumCodeSymbolsDC * sizeof(uint32)); return true; }
  void countDC(int32 Code) { m_SymbolCount[Code]++; }
};

class xHuffCounterAC
{
protected:
  uint32 m_SymbolCount[xJPEG_Constants::c_MaxNumCodeSymbolsAC];

public:
  bool init    (          ) { memset(m_SymbolCount, 0, xJPEG_Constants::c_MaxNumCodeSymbolsAC * sizeof(uint32)); return true; }
  void countAC (int32 Code) { m_SymbolCount[Code]++; }
  void countZRL(          ) { m_SymbolCount[0xF0]++; }
  void countEOB(          ) { m_SymbolCount[0x00]++; }
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG