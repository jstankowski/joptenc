/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xBitstream.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xHuffDefaultConstants
{
protected:
  //default huffman codes 
  //DC code = NumBitsDC
  //AC code = (RunLength << 4) + NumBitsAC
  static const uint8  c_HuffLenDefaultLumaDC     [ 12];
  static const uint16 c_HuffCodeDefaultLumaDC    [ 12];
  static const uint8  c_HuffLenDefaultLumaAC     [160];
  static const uint16 c_HuffCodeDefaultLumaAC    [160];

  static const uint8  c_HuffLenDefaultChromaDC   [ 12];
  static const uint16 c_HuffCodeDefaultChromaDC  [ 12];
  static const uint8  c_HuffLenDefaultChromaAC   [160];
  static const uint16 c_HuffCodeDefaultChromaAC  [160];

  static constexpr uint8  c_HuffLenDefaultLumaZRL    = 11;
  static constexpr uint16 c_HuffCodeDefaultLumaZRL   = 0x07f9;
  static constexpr uint8  c_HuffLenDefaultLumaEOB    = 4;
  static constexpr uint16 c_HuffCodeDefaultLumaEOB   = 0x000a;

  static constexpr uint8  c_HuffLenDefaultChromaZRL  = 10;
  static constexpr uint16 c_HuffCodeDefaultChromaZRL = 0x03fa;
  static constexpr uint8  c_HuffLenDefaultChromaEOB  = 2;
  static constexpr uint16 c_HuffCodeDefaultChromaEOB = 0x0000;

protected:
  static inline int32 xCalcCodeAC(int32 RunLength, int32 NumBits) { return (RunLength * 10) + NumBits - 1; }
};

//=====================================================================================================================================================================================

class xHuffDefaultEncoder : public xHuffDefaultConstants
{
public:
  static inline void  writeLumaDC   (xBitstreamWriter* Bitstream,                  int32 NumBits, uint32 Remainder) { Bitstream->writeBits(c_HuffCodeDefaultLumaDC  [NumBits], c_HuffLenDefaultLumaDC  [NumBits]); Bitstream->writeBits(Remainder, NumBits); }
  static inline void  writeLumaAC   (xBitstreamWriter* Bitstream, int32 RunLength, int32 NumBits, uint32 Remainder) { int32 Code = xCalcCodeAC(RunLength, NumBits); Bitstream->writeBits(c_HuffCodeDefaultLumaAC[Code], c_HuffLenDefaultLumaAC[Code]); Bitstream->writeBits(Remainder, NumBits); }
  static inline void  writeLumaZRL  (xBitstreamWriter* Bitstream                                                  ) { Bitstream->writeBits(c_HuffCodeDefaultLumaZRL, c_HuffLenDefaultLumaZRL); }
  static inline void  writeLumaEOB  (xBitstreamWriter* Bitstream                                                  ) { Bitstream->writeBits(c_HuffCodeDefaultLumaEOB, c_HuffLenDefaultLumaEOB); }

  static inline void  writeChromaDC (xBitstreamWriter* Bitstream,                  int32 NumBits, uint32 Remainder) { Bitstream->writeBits(c_HuffCodeDefaultChromaDC  [NumBits], c_HuffLenDefaultChromaDC  [NumBits]); Bitstream->writeBits(Remainder, NumBits); }
  static inline void  writeChromaAC (xBitstreamWriter* Bitstream, int32 RunLength, int32 NumBits, uint32 Remainder) { int32 Code = xCalcCodeAC(RunLength, NumBits); Bitstream->writeBits(c_HuffCodeDefaultChromaAC[Code], c_HuffLenDefaultChromaAC[Code]); Bitstream->writeBits(Remainder, NumBits); }
  static inline void  writeChromaZRL(xBitstreamWriter* Bitstream                                                  ) { Bitstream->writeBits(c_HuffCodeDefaultChromaZRL, c_HuffLenDefaultChromaZRL); }
  static inline void  writeChromaEOB(xBitstreamWriter* Bitstream                                                  ) { Bitstream->writeBits(c_HuffCodeDefaultChromaEOB, c_HuffLenDefaultChromaEOB); }
};

//=====================================================================================================================================================================================

class xHuffDefaultEstimator : public xHuffDefaultConstants
{
public:
  static inline int32 calcLumaDC (int32 NumBits) { return c_HuffLenDefaultLumaDC  [NumBits] + NumBits; }
  static inline int32 calcLumaAC (int32 RunLength, int32 NumBits) { int32 Code = xCalcCodeAC(RunLength, NumBits); return c_HuffLenDefaultLumaAC[Code] + NumBits; }
  static inline int32 calcLumaZRL() { return c_HuffLenDefaultLumaZRL; }
  static inline int32 calcLumaEOB() { return c_HuffLenDefaultLumaEOB; }

  static inline int32 calcChromaDC (int32 NumBits) { return c_HuffLenDefaultChromaDC[NumBits] + NumBits; }
  static inline int32 calcChromaAC (int32 RunLength, int32 NumBits) { int32 Code = xCalcCodeAC(RunLength, NumBits); return c_HuffLenDefaultChromaAC[Code] + NumBits; }
  static inline int32 calcChromaZRL() { return c_HuffLenDefaultChromaZRL; }
  static inline int32 calcChromaEOB() { return c_HuffLenDefaultChromaEOB; }
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG