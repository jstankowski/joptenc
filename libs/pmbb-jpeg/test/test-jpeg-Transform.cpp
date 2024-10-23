/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <functional>
#include <utility>
#include <array>
#include "xTestUtils.h"
#include "xMemory.h"
#include "xCommonDefJPEG.h"
#include "xJPEG_Transform.h"
#include "xJPEG_TransformConstants.h"

using namespace PMBB_NAMESPACE;
using namespace PMBB_NAMESPACE::JPEG;

//===============================================================================================================================================================================================================

constexpr int32 BA = xJPEG_Constants::c_BlockArea;

void testTransformSTD()
{
  constexpr int32 NumIters = 1024;

  std::array<uint16, BA> Src;
  std::array< int16, BA> TrC_FLT;
  std::array< int16, BA> TrC_M16;
  std::array< int16, BA> TrC_BTF;
  std::array<uint16, BA> Rec_FLT;
  std::array<uint16, BA> Rec_M16;
  std::array<uint16, BA> Rec_BTF;

  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 j = 0; j < NumIters; j++)
  {
    State = xTestUtils::fillRandom(Src.data(), NOT_VALID, BA, 1, 8, State);

    xTransformFLT::FwdTransformDCT_8x8_MFL(TrC_FLT.data(), Src.data());
    xTransformSTD::FwdTransformDCT_8x8_M16(TrC_M16.data(), Src.data());
    xTransformSTD::FwdTransformDCT_8x8_BTF(TrC_BTF.data(), Src.data());
    CHECK(xTestUtils::isSimilarBuffer(TrC_M16.data(), TrC_FLT.data(), BA, (int16)2, true));
    CHECK(xTestUtils::isSimilarBuffer(TrC_BTF.data(), TrC_FLT.data(), BA, (int16)2, true));

    for(int32 i = 0; i < BA; i++) { TrC_FLT[i] = TrC_FLT[i] / 16; }
    for(int32 i = 0; i < BA; i++) { TrC_M16[i] = TrC_M16[i] / 16; }
    for(int32 i = 0; i < BA; i++) { TrC_BTF[i] = TrC_BTF[i] / 16; }

    xTransformFLT::InvTransformDCT_8x8_MFL(Rec_FLT.data(), TrC_M16.data());
    xTransformSTD::InvTransformDCT_8x8_M16(Rec_M16.data(), TrC_M16.data());
    xTransformSTD::InvTransformDCT_8x8_BTF(Rec_BTF.data(), TrC_M16.data());
    CHECK(xTestUtils::isSimilarBuffer(Rec_M16.data(), Rec_FLT.data(), BA, (uint16)1 , true));
    CHECK(xTestUtils::isSimilarBuffer(Rec_BTF.data(), Rec_FLT.data(), BA, (uint16)1 , true));

    xTransformSTD::InvTransformDCT_8x8_M16(Rec_FLT.data(), TrC_FLT.data());
    xTransformSTD::InvTransformDCT_8x8_M16(Rec_M16.data(), TrC_M16.data());
    xTransformSTD::InvTransformDCT_8x8_BTF(Rec_BTF.data(), TrC_BTF.data());

    CHECK(xTestUtils::isSimilarBuffer(Rec_FLT.data(), Src.data(), BA, (uint16)2, true));
    CHECK(xTestUtils::isSimilarBuffer(Rec_M16.data(), Src.data(), BA, (uint16)2, true));
    CHECK(xTestUtils::isSimilarBuffer(Rec_BTF.data(), Src.data(), BA, (uint16)2, true));
  }
}

void testTransformSIMD(std::function <void(int16*, const uint16*)>RefTr, std::function <void(uint16*, const int16*)>RefInvTr,
                       std::function <void(int16*, const uint16*)>TstTr, std::function <void(uint16*, const int16*)>TstInvTr)
{
  constexpr int32 NumIters = 1024;

  std::array<uint16, BA> Src;
  std::array< int16, BA> TrC_Ref;
  std::array< int16, BA> TrC_Tst;
  std::array<uint16, BA> Rec_Ref;
  std::array<uint16, BA> Rec_Tst;

  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 j = 0; j < NumIters; j++)
  {
    State = xTestUtils::fillRandom(Src.data(), NOT_VALID, BA, 1, 8, State);

    RefTr(TrC_Ref.data(), Src.data());
    TstTr(TrC_Tst.data(), Src.data());
    CHECK(xTestUtils::isSameBuffer(TrC_Ref.data(), TrC_Tst.data(), BA, true));

    for(int32 i = 0; i < BA; i++) { TrC_Ref[i] = TrC_Ref[i] / 16; }
    for(int32 i = 0; i < BA; i++) { TrC_Tst[i] = TrC_Tst[i] / 16; }

    RefInvTr(Rec_Ref.data(), TrC_Ref.data());
    TstInvTr(Rec_Tst.data(), TrC_Tst.data());
    CHECK(xTestUtils::isSameBuffer(Rec_Ref.data(), Rec_Tst.data(), BA, true));
  }
}

std::tuple<flt64, flt64> perfTransform(std::function <void(int16*, const uint16*)>RefTr, std::function <void(uint16*, const int16*)>RefInvTr,
                                       std::function <void(int16*, const uint16*)>TstTr, std::function <void(uint16*, const int16*)>TstInvTr)
{
  constexpr int32 NumIters = 4;
  constexpr int32 NumBlock = 1024 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);
  uint16* Src = (uint16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16*  Tmp = ( int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  uint16* Dst = (uint16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  uint16* Ref = (uint16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xTestUtils::fillRandom((uint16*)Src, NOT_VALID, NumPels, 1, 8);
  
  //reference
  for(int32 i = 0; i < NumPels; i += BA) { RefTr   (Tmp + i, Src + i); }
  for(int32 i = 0; i < NumPels; i++    ) { Tmp[i] = Tmp[i] / 16; }
  for(int32 i = 0; i < NumPels; i += BA) { RefInvTr(Ref + i, Tmp + i); }

  tDuration FT = (tDuration)0;
  tDuration IT = (tDuration)0;

  //warmup  
  for(int32 i = 0; i < NumPels; i+= BA) { TstTr   (Tmp + i, Src + i); }
  for(int32 i = 0; i < NumPels; i++   ) { Tmp[i] = Tmp[i] / 16; }
  for(int32 i = 0; i < NumPels; i+= BA) { TstInvTr(Dst + i, Tmp + i); }
  CHECK(xTestUtils::isSameBuffer(Dst, Ref, NumPels, true));

  //measure
  for(int32 j = 0; j < NumIters; j++)
  {
    tTimePoint T0 = tClock::now();
    for(int32 i = 0; i < NumPels; i += BA) { TstTr(Tmp + i, Src + i); }
    tTimePoint T1 = tClock::now();
    for(int32 i = 0; i < NumPels; i++) { Tmp[i] = Tmp[i] / 16; }
    tTimePoint T2 = tClock::now();
    for(int32 i = 0; i < NumPels; i += BA) { TstInvTr(Dst + i, Tmp + i); }
    tTimePoint T3 = tClock::now();
    CHECK(xTestUtils::isSameBuffer(Dst, Ref, NumPels, true));
    FT += T1 - T0;
    IT += T3 - T2;
  }

  int64 NumBytes      = BuffSize * NumIters;
  flt64 BytesPerSecFT = NumBytes / std::chrono::duration_cast<tDurationS>(FT).count();
  flt64 BytesPerSecIT = NumBytes / std::chrono::duration_cast<tDurationS>(IT).count();
 
  xMemory::xAlignedFree(Src);
  xMemory::xAlignedFree(Tmp);
  xMemory::xAlignedFree(Dst);
  xMemory::xAlignedFree(Ref);

  return { BytesPerSecFT, BytesPerSecIT };
}

//===============================================================================================================================================================================================================

TEST_CASE("xTransformSTD")
{
  testTransformSTD();
}

#if X_SIMD_CAN_USE_SSE
TEST_CASE("xTransformSSE_M16")
{
  testTransformSIMD(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformSSE::FwdTransformDCT_8x8_M16, xTransformSSE::InvTransformDCT_8x8_M16);
}
#endif //X_SIMD_CAN_USE_SSE

#if X_SIMD_CAN_USE_AVX
TEST_CASE("xTransformAVX_M16")
{
  testTransformSIMD(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformAVX::FwdTransformDCT_8x8_M16, xTransformAVX::InvTransformDCT_8x8_M16);
}
#endif //X_SIMD_CAN_USE_AVC

#if X_SIMD_CAN_USE_AVX512
TEST_CASE("xTransformAVX512_M16")
{
  testTransformSIMD(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformAVX512::FwdTransformDCT_8x8_M16, xTransformAVX512::InvTransformDCT_8x8_M16);
}
#endif //X_SIMD_CAN_USE_AVX512

//===============================================================================================================================================================================================================

#ifdef NDEBUG 

TEST_CASE("xTransformSTD_FLT-perf")
{
  auto [FT, IT] = perfTransform(xTransformFLT::FwdTransformDCT_8x8_MFL, xTransformFLT::InvTransformDCT_8x8_MFL, xTransformFLT::FwdTransformDCT_8x8_MFL, xTransformFLT::InvTransformDCT_8x8_MFL);
  fmt::print("TIME(xTransformFLT::FwdTransformDCT_8x8_MFL) = {:.2f} MiB/s\n", FT / (1024 * 1024));
  fmt::print("TIME(xTransformFLT::InvTransformDCT_8x8_MFL) = {:.2f} MiB/s\n", IT / (1024 * 1024));
}

TEST_CASE("xTransformSTD_M16-perf")
{
  auto [FT, IT] = perfTransform(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16);
  fmt::print("TIME(xTransformSTD::FwdTransformDCT_8x8_M16) = {:.2f} MiB/s\n", FT / (1024 * 1024));
  fmt::print("TIME(xTransformSTD::InvTransformDCT_8x8_M16) = {:.2f} MiB/s\n", IT / (1024 * 1024));
}

TEST_CASE("xTransformSTD_BTF-perf")
{
  auto [FT, IT] = perfTransform(xTransformSTD::FwdTransformDCT_8x8_BTF, xTransformSTD::InvTransformDCT_8x8_BTF, xTransformSTD::FwdTransformDCT_8x8_BTF, xTransformSTD::InvTransformDCT_8x8_BTF);
  fmt::print("TIME(xTransformSTD::FwdTransformDCT_8x8_BTF) = {:.2f} MiB/s\n", FT / (1024 * 1024));
  fmt::print("TIME(xTransformSTD::InvTransformDCT_8x8_BTF) = {:.2f} MiB/s\n", IT / (1024 * 1024));
}

#if X_SIMD_CAN_USE_SSE
TEST_CASE("xTransformSSE_M16-perf")
{
  auto [FT, IT] = perfTransform(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformSSE::FwdTransformDCT_8x8_M16, xTransformSSE::InvTransformDCT_8x8_M16);
  fmt::print("TIME(xTransformSSE::FwdTransformDCT_8x8_M16) = {:.2f} MiB/s\n", FT / (1024 * 1024));
  fmt::print("TIME(xTransformSSE::InvTransformDCT_8x8_M16) = {:.2f} MiB/s\n", IT / (1024 * 1024));
}
#endif

#if X_SIMD_CAN_USE_AVX
TEST_CASE("xTransformAVX_M16-perf")
{
  auto [FT, IT] = perfTransform(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformAVX::FwdTransformDCT_8x8_M16, xTransformAVX::InvTransformDCT_8x8_M16);
  fmt::print("TIME(xTransformAVX::FwdTransformDCT_8x8_M16) = {:.2f} MiB/s\n", FT / (1024 * 1024));
  fmt::print("TIME(xTransformAVX::InvTransformDCT_8x8_M16) = {:.2f} MiB/s\n", IT / (1024 * 1024));
}
#endif

#if X_SIMD_CAN_USE_AVX512
TEST_CASE("xTransformAVX512_M16-perf")
{
  auto [FT, IT] = perfTransform(xTransformSTD::FwdTransformDCT_8x8_M16, xTransformSTD::InvTransformDCT_8x8_M16, xTransformAVX512::FwdTransformDCT_8x8_M16, xTransformAVX512::InvTransformDCT_8x8_M16);
  fmt::print("TIME(xTransformAVX512::FwdTransformDCT_8x8_M16) = {:.2f} MiB/s\n", FT / (1024 * 1024));
  fmt::print("TIME(xTransformAVX512::InvTransformDCT_8x8_M16) = {:.2f} MiB/s\n", IT / (1024 * 1024));
}
#endif

#endif //def NDEBUG

//===============================================================================================================================================================================================================

