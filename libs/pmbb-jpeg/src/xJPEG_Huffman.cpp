/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Huffman.h"

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================
// xHuffCommon
//=====================================================================================================================================================================================
bool xHuffCommon::xInitEncoderTables(uint32* HuffCode, uint8* HuffLen, const xJFIF::xHuffTable& HuffTable)
{
  const xJFIF::xHuffTable::tCodeL& TabCodeLengths = HuffTable.getCodeLengths();
  const xJFIF::tByteV&             TabCodeSymbols = HuffTable.getCodeSymbols();

  uint32 TmpCode[257];
  uint8  TmpLen [257];

  //generate lengths
  int32 NumLengths = xFillTmpLengths(TmpLen, TabCodeLengths);
  if(NumLengths == NOT_VALID) { return false; }

  //generate codes
  bool Result = xFillTmpCodes(TmpCode, TmpLen, NumLengths);
  if(!Result) { return false; }

  //writeout
  int32 TabLen = HuffTable.getClass() == xJFIF::xHuffTable::eHuffClass::DC ? 16 : 256;
  memset(HuffCode, 0, TabLen*sizeof(uint32));
  memset(HuffLen , 0, TabLen*sizeof(uint8 ));

  int32 MaxSymbol = HuffTable.isDC() ? 15 : 255;
  for (int32 p = 0; p < NumLengths; p++)
  {
    int32 idx = TabCodeSymbols[p];
    if(idx < 0 || idx > MaxSymbol || HuffLen[idx]) { return false; }
    HuffCode[idx] = TmpCode[p];
    HuffLen [idx] = TmpLen [p];
  }

  return true;
}
int32 xHuffCommon::xFillTmpLengths(uint8* Lenghts, const xJFIF::xHuffTable::tCodeL& TabCodeLengths)
{
  int32 p = 0;
  for(int32 l = 1; l <= 16; l++)
  {
    int32 i = TabCodeLengths[l - 1];
    if(i < 0 || p + i > 256) { return NOT_VALID; }
    while(i--) { Lenghts[p++] = (uint8)l; }
  }
  Lenghts[p] = 0;

  return p;
}
int32 xHuffCommon::xFillTmpCodes(uint32* Codes, const uint8* Lenghts, int32 /*NumLengths*/)
{
  uint32 Code = 0;
  uint32 si   = Lenghts[0];
  int32  p    = 0;
  while(Lenghts[p])
  {
    while(Lenghts[p] == si)
    {      
      Codes[p++] = Code;
      Code++;
    }
    if(Code >= ((uint32)1 << si)) { return false; }
    Code <<= 1;
    si++;
  }
  return true;
}

//=============================================================================================================================================================================
// xHuffDecoder
//=====================================================================================================================================================================================
bool xHuffDecoder::xInitTables(const xJFIF::xHuffTable& HuffTable)
{
  const xJFIF::xHuffTable::tCodeL& TabCodeLengths = HuffTable.getCodeLengths();
  const xJFIF::tByteV&               TabCodeSymbols = HuffTable.getCodeSymbols();

  uint32 TmpCode[257];
  uint8  TmpLen [257];

  //copy code symbols from HuffTable
  memset(m_CodeSymbols, 0, 256);
  memcpy(m_CodeSymbols, TabCodeSymbols.data(), TabCodeSymbols.size());

  //generate length
  int32 NumLengths = xFillTmpLengths(TmpLen, TabCodeLengths);
  if(NumLengths == NOT_VALID) { return false; }

  //generate codes
  bool Result = xFillTmpCodes(TmpCode, TmpLen, NumLengths);
  if(!Result) { return false; }

  {
    int32 p = 0;
    for(int32 l = 1; l <= 16; l++)
    {
      if(TabCodeLengths[l - 1])
      {
        m_ValOffset[l] = p - (int32)TmpCode[p];
        p += TabCodeLengths[l - 1];
        m_MaxCode[l] = TmpCode[p - 1];
      }
      else
      {
        m_MaxCode[l] = -1;
      }
    }
    m_ValOffset[17] = 0;
    m_MaxCode  [17] = 0xFFFFFL;
  }

#if X_PMBB_JPEG_MULTI_LEVEL_LOOKAHEAD
  for(int32 i = 0; i < (1 << c_LookAhead1st); i++)
  {
    m_Lookup1st[i] = (uint16)((c_LookAhead1st + 1) << c_LookAhead1st);
  }
  {
    int32 p = 0;
    for(int32 l = 1; l <= c_LookAhead1st; l++)
    {
      for(uint32 i = 1; i <= (int32)TabCodeLengths[l - 1]; i++, p++)
      {
        int32 LookBits = TmpCode[p] << (c_LookAhead1st - l);
        for(int32 ctr = 1 << (c_LookAhead1st - l); ctr > 0; ctr--)
        {
          m_Lookup1st[LookBits] = (l << c_LookAhead1st) | TabCodeSymbols[p];
          LookBits++;
        }
      }
    }
  }

  for(int32 i = 0; i < (1 << c_LookAhead2nd); i++)
  {
    m_Lookup2nd[i] = (uint32)((c_LookAhead2nd + 1) << c_LookAhead2nd);
  }
  {
    int32 p = 0;
    for(uint32 l = 1; l <= c_LookAhead2nd; l++)
    {
      for(int32 i = 1; i <= (int32)TabCodeLengths[l - 1]; i++, p++)
      {
        int32 LookBits = TmpCode[p] << (c_LookAhead2nd - l);
        for(int32 ctr = 1 << (c_LookAhead2nd - l); ctr > 0; ctr--)
        {
          m_Lookup2nd[LookBits] = ((l << c_LookAhead2nd) | TabCodeSymbols[p]);
          LookBits++;
        }
      }
    }
  }

#else

  for(int32 i = 0; i < (1 << c_LookAhead); i++)
  {
    m_Lookup[i] = (c_LookAhead + 1) << c_LookAhead;
  }
  {
    int32 p = 0;
    for(int32 l = 1; l <= c_LookAhead; l++)
    {
      for(int32 i = 1; i <= (int32)TabCodeLengths[l - 1]; i++, p++)
      {
        int32 LookBits = TmpCode[p] << (c_LookAhead - l);
        for(int32 ctr = 1 << (c_LookAhead - l); ctr > 0; ctr--)
        {
          m_Lookup[LookBits] = (l << c_LookAhead) | TabCodeSymbols[p];
          LookBits++;
        }
      }
    }
  }

#endif

  if (HuffTable.isDC())
  {
    for (int32 i = 0; i < NumLengths; i++)
    {
      int sym = TabCodeSymbols[i];
      if(sym < 0 || sym > 15) { return false; }
    }
  }

  return true;
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG