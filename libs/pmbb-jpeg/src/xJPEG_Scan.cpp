/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Scan.h"
#include "xJPEG_Constants.h"
#include "xHelpersSIMD.h"

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================
// xScanAVX512
//=============================================================================================================================================================================
#if X_SIMD_CAN_USE_AVX512
void xScanAVX512::Scan(int16* ScanCoeff, const int16* Coeff)
{
  __m512i Coeff_I16_V0     = _mm512_loadu_si512((__m512i*)(Coeff     ));
  __m512i Coeff_I16_V1     = _mm512_loadu_si512((__m512i*)(Coeff + 32));
  __m512i Selector_U16_V0  = _mm512_setr_epi16( 0,  1,  8, 16,  9,  2,  3, 10,
                                               17, 24, 32, 25, 18, 11,  4,  5,
                                               12, 19, 26, 33, 40, 48, 41, 34,
                                               27, 20, 13,  6,  7, 14, 21, 28);
  __m512i Selector_U16_V1  = _mm512_setr_epi16(35, 42, 49, 56, 57, 50, 43, 36,
                                               29, 22, 15, 23, 30, 37, 44, 51,
                                               58, 59, 52, 45, 38, 31, 39, 46,
                                               53, 60, 61, 54, 47, 55, 62, 63);
  __m512i ScanCoeff_I16_V0 = _mm512_permutex2var_epi16(Coeff_I16_V0, Selector_U16_V0, Coeff_I16_V1);
  __m512i ScanCoeff_I16_V1 = _mm512_permutex2var_epi16(Coeff_I16_V0, Selector_U16_V1, Coeff_I16_V1);
  _mm512_storeu_si512((__m512i*)(ScanCoeff     ), ScanCoeff_I16_V0);
  _mm512_storeu_si512((__m512i*)(ScanCoeff + 32), ScanCoeff_I16_V1);
}
void xScanAVX512::InvScan(int16* Coeff, const int16* ScanCoeff)
{
  __m512i ScanCoeff_I16_V0 = _mm512_loadu_si512((__m512i*)(ScanCoeff     ));
  __m512i ScanCoeff_I16_V1 = _mm512_loadu_si512((__m512i*)(ScanCoeff + 32));
  __m512i Selector_U16_V0  = _mm512_setr_epi16( 0,  1,  5,  6, 14, 15, 27, 28,
                                                2,  4,  7, 13, 16, 26, 29, 42,
                                                3,  8, 12, 17, 25, 30, 41, 43,
                                                9, 11, 18, 24, 31, 40, 44, 53);
  __m512i Selector_U16_V1  = _mm512_setr_epi16(10, 19, 23, 32, 39, 45, 52, 54,
                                               20, 22, 33, 38, 46, 51, 55, 60,
                                               21, 34, 37, 47, 50, 56, 59, 61,
                                               35, 36, 48, 49, 57, 58, 62, 63);
  __m512i Coeff_I16_V0 = _mm512_permutex2var_epi16(ScanCoeff_I16_V0, Selector_U16_V0, ScanCoeff_I16_V1);
  __m512i Coeff_I16_V1 = _mm512_permutex2var_epi16(ScanCoeff_I16_V0, Selector_U16_V1, ScanCoeff_I16_V1);
  _mm512_storeu_si512((__m512i*)(Coeff     ), Coeff_I16_V0);
  _mm512_storeu_si512((__m512i*)(Coeff + 32), Coeff_I16_V1);
}
#endif //X_SIMD_CAN_USE_AVX512

//=============================================================================================================================================================================
// xScanSTD
//=============================================================================================================================================================================
namespace {
template <uint32 First, uint32 Last> struct xcStaticFor
{
  template <typename Lambda> static inline constexpr void apply(Lambda const& f)
  {
    if constexpr(First < Last)
    {
      f(std::integral_constant<uint32, First>{});
      xcStaticFor<First + 1, Last>::apply(f);
    }
  }
};
};

void xScanSTD::Scan(int16* ScanCoeff, const int16* Coeff)
{
#if defined _MSC_VER
  for(int32 i = 0; i < 64; i++) { ScanCoeff[i] = Coeff[xJPEG_Constants::m_ScanZigZag[i]]; }  
#else
  //static loop unroll, only for not-MSVC since MSVC does not understand this idea and producess terrible assembly
  xcStaticFor<0, 64>::apply([&](uint32 i) { ScanCoeff[i] = Coeff[xJPEG_Constants::m_ScanZigZag[i]]; }); 
#endif
}
void xScanSTD::InvScan(int16* Coeff, const int16* ScanCoeff)
{
#if defined _MSC_VER
  for(int32 i = 0; i < 64; i++) { Coeff[i] = ScanCoeff[xJPEG_Constants::m_InvScanZigZag[i]]; }  
#else
  //static loop unroll, only for not-MSVC since MSVC does not understand this idea and producess terrible assembly
  xcStaticFor<0, 64>::apply([&](uint32 i) { Coeff[i] = ScanCoeff[xJPEG_Constants::m_InvScanZigZag[i]]; });
#endif
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG