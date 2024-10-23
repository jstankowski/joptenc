/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once

//=============================================================================================================================================================================
// PMBB-core
//=============================================================================================================================================================================
#include "xCommonDefCMPR.h"

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================
// FourCC and EightCC
//=============================================================================================================================================================================
constexpr uint32 xMakeFourCC (char A, char B, char C, char D                                ) { return (((uint32)A)) | (((uint32)B) << 8) | (((uint32)C) << 16) | (((uint32)D) << 24); }
constexpr uint64 xMakeEightCC(char A, char B, char C, char D, char E, char F, char G, char H) { return (((uint64)A)) | (((uint64)B) << 8) | (((uint64)C) << 16) | (((uint64)D) << 24) | (((uint64)E) << 32) | (((uint64)F) << 40) | (((uint64)G) << 48) | (((uint64)H) << 56); }
constexpr uint32 xMakeFourCC (const char* T) { return xMakeFourCC (T[0], T[1], T[2], T[3]                        ); }
constexpr uint64 xMakeEightCC(const char* T) { return xMakeEightCC(T[0], T[1], T[2], T[3], T[4], T[5], T[6], T[7]); }

//=============================================================================================================================================================================

enum class eImpl //JPEG implementation
{
  INVALID  = NOT_VALID,
  Simple   = 0,
  Advanded = 1,
};
eImpl       xStrToImpl(const std::string& Impl);
std::string xImplToStr(eImpl Impl             );

enum class eQTLa //JPEG Quantization Table Layout
{
  INVALID  = NOT_VALID,
  Default  = 0,
  Flat     = 1,
  SemiFlat = 2,
};
eQTLa       xStrToQTLa(const std::string& QTLa);
std::string xQTLaToStr(eQTLa QTLa             );

//=============================================================================================================================================================================

} //end of namespace PMBB::JPEG