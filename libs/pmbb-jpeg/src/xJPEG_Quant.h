/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJFIF.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xQuantFLT
{
public:
  static void QuantScale(int16* restrict Dst, const int16* Src, const uint16* QuantCoeff, const uint16*, const uint16*);
  static void InvScale  (int16* restrict Dst, const int16* Src, const uint16* QuantCoeff);
};

//=====================================================================================================================================================================================

class xQuantSTD
{
public:
  static void QuantScale(int16* restrict Dst, const int16* Src, const uint16* Correction, const uint16* Reciprocal, const uint16* Shift);
  static void InvScale  (int16* restrict Dst, const int16* Src, const uint16* QuantCoeff);
};

//=====================================================================================================================================================================================

#if X_SIMD_CAN_USE_SSE
#define X_CAN_USE_SSE 1
class xQuantSSE
{
public:
  static void QuantScale(int16* restrict Dst, const int16* Src, const uint16* Correction, const uint16* Reciprocal, const uint16* Scale);
  static void InvScale  (int16* restrict Dst, const int16* Src, const uint16* QuantCoeff);
};
#else //X_SIMD_CAN_USE_SSE
#define X_CAN_USE_SSE 0
#endif //X_SIMD_CAN_USE_SSE

//=====================================================================================================================================================================================

#if X_SIMD_CAN_USE_AVX
#define X_CAN_USE_AVX 1
class xQuantAVX
{
public:
  static void QuantScale(int16* restrict Dst, const int16* Src, const uint16* Correction, const uint16* Reciprocal, const uint16* Scale);
  static void InvScale  (int16* restrict Dst, const int16* Src, const uint16* QuantCoeff);
};
#else //X_SIMD_CAN_USE_AVX
#define X_CAN_USE_AVX 0
#endif //X_SIMD_CAN_USE_AVX

//=====================================================================================================================================================================================

#if X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 1
class xQuantAVX512
{
public:
  static void QuantScale(int16* restrict Dst, const int16* Src, const uint16* Correction, const uint16* Reciprocal, const uint16* Scale);
  static void InvScale  (int16* restrict Dst, const int16* Src, const uint16* QuantCoeff);
};
#else //X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 0
#endif //X_SIMD_CAN_USE_AVX512

//=====================================================================================================================================================================================

class xQuantizer
{
protected:
  uint16 m_QuantCoeff[64];

  uint16 m_Reciprocal[64];
  uint16 m_Correction[64];
  uint16 m_Scale     [64];
  uint16 m_Shift     [64];

public:
  void Init(eCmp Cmp, int32 Quality, eQTLa QuantTabLayout = eQTLa::Default);
  void Init(const xJFIF::xQuantTable& QuantTable);

  std::string FormatCoeffs(const std::string& Prefix) const;

#if X_CAN_USE_AVX512
  void QuantScale(int16* Dst, const int16* Src) const { xQuantAVX512::QuantScale(Dst, Src, m_Correction, m_Reciprocal, m_Scale); }
  void InvScale  (int16* Dst, const int16* Src) const { xQuantAVX512::InvScale  (Dst, Src, m_QuantCoeff                       ); }
#elif X_CAN_USE_AVX
  void QuantScale(int16* Dst, const int16* Src) const { xQuantAVX   ::QuantScale(Dst, Src, m_Correction, m_Reciprocal, m_Scale); }
  void InvScale  (int16* Dst, const int16* Src) const { xQuantAVX   ::InvScale  (Dst, Src, m_QuantCoeff                       ); }
#elif X_CAN_USE_SSE
  void QuantScale(int16* Dst, const int16* Src) const { xQuantSSE   ::QuantScale(Dst, Src, m_Correction, m_Reciprocal, m_Scale); }
  void InvScale  (int16* Dst, const int16* Src) const { xQuantSSE   ::InvScale  (Dst, Src, m_QuantCoeff                       ); }
#else
  void QuantScale(int16* Dst, const int16* Src) const { xQuantSTD   ::QuantScale(Dst, Src, m_Correction, m_Reciprocal, m_Shift); }
  void InvScale  (int16* Dst, const int16* Src) const { xQuantSTD   ::InvScale  (Dst, Src, m_QuantCoeff                       ); }
#endif

protected:
  void  xInit(const uint8* QuantTable);

  static int32 xComputeReciprocal(uint16 Divisor, uint16& Reciprocal, uint16& Correction, uint16& Scale, uint16& Shift);
};

//=====================================================================================================================================================================================

class xQuantizerSet
{
protected:
  xQuantizer m_Quantizers[xJPEG_Constants::c_MaxQuantTabs];

public:
  void  Init(int32 QuantTableIdx, eCmp Cmp, int32 Quality, eQTLa QuantTabLayout = eQTLa::Default);
  void  Init(std::vector<xJFIF::xQuantTable>& QuantTables);

  const xQuantizer& getQuantizer(int32 QuantTableId) const { return m_Quantizers[QuantTableId]; }

  void  QuantScale(int16* Dst, const int16* Src, int32 QuantTableId) { m_Quantizers[QuantTableId].QuantScale(Dst, Src); }
  void  InvScale  (int16* Dst, const int16* Src, int32 QuantTableId) { m_Quantizers[QuantTableId].InvScale  (Dst, Src); }
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG