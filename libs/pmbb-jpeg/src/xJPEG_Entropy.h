/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJFIF.h"
#include "xJPEG_Huffman.h"
#include "xBitstream.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xEntropyCommon
{
protected:
  int16  m_LastDC[xJPEG_Constants::c_MaxComponents];

public:
  uint32 getLastDC   (eCmp   Cmp) const { return m_LastDC[(uint32)Cmp]; }

protected:
  static uint32 xNumBits    (uint32 Val) { return 32 - xLZCNT(Val);  }
  void   xResetLastDC(          ) { memset(m_LastDC, 0, sizeof(m_LastDC)); }

#if X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 1
  static int32 findLastNonZeroAVX512(const int16* ScanCoeff);
#else //X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 0
#endif //X_SIMD_CAN_USE_AVX512
    
  static int32 findLastNonZeroSTD(const int16* ScanCoeff);

public:
#if X_CAN_USE_AVX512
  static inline int32 findLastNonZero(const int16* ScanCoeff) { return findLastNonZeroAVX512(ScanCoeff); }
#else
  static inline int32 findLastNonZero(const int16* ScanCoeff) { return findLastNonZeroSTD   (ScanCoeff); }
#endif
};

//=====================================================================================================================================================================================

class xEntropyDecoder : public xEntropyCommon
{
protected:
  xHuffDecoder*    m_HuffDecoderDC[xJPEG_Constants::c_MaxHuffTabs];
  xHuffDecoder*    m_HuffDecoderAC[xJPEG_Constants::c_MaxHuffTabs];
  xBitstreamReader m_Bitstream;

public:
  xEntropyDecoder () { memset(m_HuffDecoderDC, 0, sizeof(m_HuffDecoderDC)); memset(m_HuffDecoderAC, 0, sizeof(m_HuffDecoderAC)); }
  ~xEntropyDecoder() { UnInit(); }
  bool Init  (std::vector<xJFIF::xHuffTable>& HuffTables);
  void UnInit();

  void StartSlice (xByteBuffer* ByteBuffer);
  void FinishSlice();
  void DecodeBlock(int16* ScanCoeff, eCmp Cmp, int32 HuffTableIdDC, int32 HuffTableIdAC);
};

//=====================================================================================================================================================================================

class xEntropyEncoder : public xEntropyCommon
{
protected:
  xHuffEncoderDC*  m_HuffEncoderDC[xJPEG_Constants::c_MaxHuffTabs];
  xHuffEncoderAC*  m_HuffEncoderAC[xJPEG_Constants::c_MaxHuffTabs];
  xBitstreamWriter m_Bitstream;

public:
  xEntropyEncoder () { memset(m_HuffEncoderDC, 0, sizeof(m_HuffEncoderDC)); memset(m_HuffEncoderAC, 0, sizeof(m_HuffEncoderAC)); }
  ~xEntropyEncoder() { UnInit(); }
  bool  Init  (std::vector<xJFIF::xHuffTable>& HuffTables);
  void  UnInit();

  void  StartSlice (xByteBuffer* ByteBuffer);
  void  FinishSlice();
  void  EncodeBlock(const int16* ScanCoeff, eCmp Cmp, int32 HuffTableIdDC, int32 HuffTableIdAC);
};

//=====================================================================================================================================================================================

class xEntropyEncoderDefault : public xEntropyCommon
{
protected:
  xBitstreamWriter m_Bitstream;

public:
  xEntropyEncoderDefault () { }
  ~xEntropyEncoderDefault() { }
  bool  Init  (std::vector<xJFIF::xHuffTable>& /*HuffTables*/) { return true; }
  void  UnInit() {}

  void  StartSlice (xByteBuffer* ByteBuffer);
  void  FinishSlice();
  void  EncodeBlock(int16* ScanCoeff, eCmp Cmp) { Cmp == eCmp::LM ? xEncodeBlockL(ScanCoeff) : xEncodeBlockC(ScanCoeff, Cmp); }

protected:
  void  xEncodeBlockL(const int16* ScanCoeff          );
  void  xEncodeBlockC(const int16* ScanCoeff, eCmp Cmp);
};

//=====================================================================================================================================================================================

class xEntropyEstimator : public xEntropyCommon
{
protected:
  xHuffEstimatorDC*  m_HuffEstimatorDC[xJPEG_Constants::c_MaxHuffTabs];
  xHuffEstimatorAC*  m_HuffEstimatorAC[xJPEG_Constants::c_MaxHuffTabs];

public:
  xEntropyEstimator () { memset(m_HuffEstimatorDC, 0, sizeof(m_HuffEstimatorDC)); memset(m_HuffEstimatorAC, 0, sizeof(m_HuffEstimatorAC)); }
  ~xEntropyEstimator() { UnInit(); }
  bool  Init  (std::vector<xJFIF::xHuffTable>& HuffTables);
  void  UnInit();

  void  StartSlice() { xResetLastDC(); }
  int32 EstimateBlock(const int16* ScanCoeff, eCmp Cmp, int32 HuffTableIdDC, int32 HuffTableIdAC);
  int32 EstimateBlockStateless(const int16* ScanCoeff, int32 LastDC, int32 HuffTableIdDC, int32 HuffTableIdAC) const;

protected:
  int32 xEstimateBlockCommon(const int16* ScanCoeff, int32 DeltaDC, int32 HuffTableIdDC, int32 HuffTableIdAC) const;
};

//=====================================================================================================================================================================================

class xEntropyEstimatorDefault : public xEntropyCommon
{
public:
  xEntropyEstimatorDefault () { }
  ~xEntropyEstimatorDefault() { UnInit(); }
  bool  Init  (std::vector<xJFIF::xHuffTable>& /*HuffTables*/) { return true; };
  void  UnInit() {};

  void  StartSlice() { xResetLastDC(); }
  int32 EstimateBlock(const int16* ScanCoeff, eCmp Cmp);
  static int32 EstimateBlockStateless(const int16* ScanCoeff, int32 LastDC);
protected:
  static inline int32 xEstimateBlockStatelessL(const int16* ScanCoeff, int32 LastDC ) { return xEstimateBlockCommonL(ScanCoeff, ScanCoeff[0] - LastDC); }
  static inline int32 xEstimateBlockStatelessC(const int16* ScanCoeff, int32 LastDC ) { return xEstimateBlockCommonC(ScanCoeff, ScanCoeff[0] - LastDC); }
  static int32        xEstimateBlockCommonL   (const int16* ScanCoeff, int32 DeltaDC);
  static int32        xEstimateBlockCommonC   (const int16* ScanCoeff, int32 DeltaDC);
};

//=====================================================================================================================================================================================

class xEntropyCounter : public xEntropyCommon
{
protected:
  xHuffCounterDC*  m_HuffCounterDC[xJPEG_Constants::c_MaxHuffTabs];
  xHuffCounterAC*  m_HuffCounterAC[xJPEG_Constants::c_MaxHuffTabs];

public:
  xEntropyCounter () { memset(m_HuffCounterDC, 0, sizeof(m_HuffCounterDC)); memset(m_HuffCounterAC, 0, sizeof(m_HuffCounterAC)); }
  ~xEntropyCounter() { UnInit(); }
  bool  Init  (std::vector<xJFIF::xHuffTable>& HuffTables);
  void  UnInit();

  void  CountBlock(const int16* ScanCoeff, int32 LastDC, int32 HuffTableIdDC, int32 HuffTableIdAC);
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG