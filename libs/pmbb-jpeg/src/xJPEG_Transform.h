/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"

namespace PMBB_NAMESPACE::JPEG {

//===============================================================================================================================================================================================================

class xTransformFLT
{
public:
  //direct multiplication with flt32 transform coefficients
  static void FwdTransformDCT_8x8_MFL(int16* restrict Dst, const uint16* Src);
  static void InvTransformDCT_8x8_MFL(uint16* restrict Dst, const int16* Src);
};

//===============================================================================================================================================================================================================

class xTransformSTD
{
public:
  //direct multiplication with 16 bit transform coefficients, HEVC style full range 16 bit output
  static void FwdTransformDCT_8x8_M16(int16*  restrict Dst, const uint16* Src);
  static void InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16*  Src);
  
  //fancy butterfly method using 13 bit integer coefficients, HEVC style full range 16 bit output
  static void FwdTransformDCT_8x8_BTF(int16*  restrict Dst, const uint16* Src);
  static void InvTransformDCT_8x8_BTF(uint16* restrict Dst, const int16*  Src);
};

//===============================================================================================================================================================================================================

#if X_SIMD_CAN_USE_SSE
#define X_CAN_USE_SSE 1
class xTransformSSE
{
public:
  //SSE direct multiplication with 8 bit transform coefficients, HEVC style full range 16 bit output
  static void FwdTransformDCT_8x8_M16(int16*  restrict Dst, const uint16* Src);
  static void InvTransformDCT_8x8_M16(uint16* restrict Dst, const  int16* Src);

};
#else //X_SIMD_CAN_USE_SSE
#define X_CAN_USE_SSE 0
#endif //X_SIMD_CAN_USE_SSE

//===============================================================================================================================================================================================================

#if X_SIMD_CAN_USE_AVX
#define X_CAN_USE_AVX 1
class xTransformAVX
{
public:
  //AVX direct multiplication with 8 bit transform coefficients, HEVC style full range 16 bit output
  static void FwdTransformDCT_8x8_M16(int16*  restrict Dst, const uint16* Src);
  static void InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16*  Src);
};
#else //X_SIMD_CAN_USE_AVX
#define X_CAN_USE_AVX 0
#endif //X_SIMD_CAN_USE_AVX

//===============================================================================================================================================================================================================

#if X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 1
class xTransformAVX512
{
public:
  //AVX512 direct multiplication with 8 bit transform coefficients, HEVC style full range 16 bit output
  static void FwdTransformDCT_8x8_M16(int16*  restrict Dst, const uint16* Src);
  static void InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16*  Src);
};
#else //X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 0
#endif //X_SIMD_CAN_USE_AVX512

//===============================================================================================================================================================================================================

class xTransform
{
public:
#if X_CAN_USE_AVX512
  static void FwdTransformDCT_8x8(int16*  Dst, const uint16* Src) { xTransformAVX512::FwdTransformDCT_8x8_M16(Dst, Src); }
  static void InvTransformDCT_8x8(uint16* Dst, const int16*  Src) { xTransformAVX512::InvTransformDCT_8x8_M16(Dst, Src); }
#elif X_CAN_USE_AVX
  static void FwdTransformDCT_8x8(int16*  Dst, const uint16* Src) { xTransformAVX::FwdTransformDCT_8x8_M16(Dst, Src); }
  static void InvTransformDCT_8x8(uint16* Dst, const int16*  Src) { xTransformSSE::InvTransformDCT_8x8_M16(Dst, Src); }
#elif X_CAN_USE_SSE
  static void FwdTransformDCT_8x8(int16*  Dst, const uint16* Src) { xTransformSSE::FwdTransformDCT_8x8_M16(Dst, Src); }
  static void InvTransformDCT_8x8(uint16* Dst, const int16*  Src) { xTransformSSE::InvTransformDCT_8x8_M16(Dst, Src); }
#else
  static void FwdTransformDCT_8x8(int16*  Dst, const uint16* Src) { xTransformSTD::FwdTransformDCT_8x8_BTF(Dst, Src); }
  static void InvTransformDCT_8x8(uint16* Dst, const int16*  Src) { xTransformSTD::InvTransformDCT_8x8_BTF(Dst, Src); }
#endif
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB::JPEG

#undef X_CAN_USE_SSE
#undef X_CAN_USE_AVX
#undef X_CAN_USE_AVX512

