/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================

#if X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 1
class xScanAVX512
{
public:
  static void Scan   (int16* ScanCoeff, const int16* Coeff    );
  static void InvScan(int16* Coeff,     const int16* ScanCoeff);
};
#else //X_SIMD_CAN_USE_AVX512
#define X_CAN_USE_AVX512 0
#endif //X_SIMD_CAN_USE_AVX512

//=============================================================================================================================================================================

class xScanSTD
{
public:
  static void Scan   (int16* ScanCoeff, const int16* Coeff    );
  static void InvScan(int16* Coeff,     const int16* ScanCoeff);
};

//=============================================================================================================================================================================

class xScan
{
public:
#if X_CAN_USE_AVX512
  static inline void Scan   (int16* ScanCoeff, const int16* Coeff    ) { xScanAVX512::Scan   (ScanCoeff, Coeff); }
  static inline void InvScan(int16* Coeff,     const int16* ScanCoeff) { xScanAVX512::InvScan(Coeff, ScanCoeff); }
#else
  static inline void Scan   (int16* ScanCoeff, const int16* Coeff    ) { xScanSTD::Scan   (ScanCoeff, Coeff); }
  static inline void InvScan(int16* Coeff,     const int16* ScanCoeff) { xScanSTD::InvScan(Coeff, ScanCoeff); }
#endif
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG