/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <functional>
#include <utility>
#include <array>
#include <tuple>
#include "xTestUtils.h"
#include "xMemory.h"
#include "xCommonDefJPEG.h"
#include "xJPEG_Scan.h"
#include "xJPEG_Constants.h"

using namespace PMBB_NAMESPACE;
using namespace PMBB_NAMESPACE::JPEG;

constexpr int32 BA = xJPEG_Constants::c_BlockArea;

//===============================================================================================================================================================================================================

void testScan(std::function <void(int16*, const int16*)>Scan, std::function <void(int16*, const int16*)>InvScan)
{  
  constexpr std::array<int16, BA> TmpSequence   = [] { std::array<int16, BA> R{}; for(size_t i = 0; i < R.size(); i++) { R[i] = (int16)i                        ; } return R; }();
  constexpr std::array<int16, BA> TmpScanZigZag = [] { std::array<int16, BA> R{}; for(size_t i = 0; i < R.size(); i++) { R[i] = xJPEG_Constants::m_ScanZigZag[i]; } return R; }();

  std::array<int16, BA> Src = { 0 };
  std::array<int16, BA> Dst = { 0 };

  CAPTURE("Forward Scan");
  Src = TmpSequence;
  Scan(Dst.data(), Src.data());
  CHECK(xTestUtils::isSameBuffer(Dst.data(), TmpScanZigZag.data(), BA, true));

  CAPTURE("Inverse Scan");
  Src = TmpScanZigZag;
  InvScan(Dst.data(), Src.data());
  CHECK(xTestUtils::isSameBuffer(Dst.data(), TmpSequence.data(), BA, true));
}

std::tuple<flt64, flt64> perfScan(std::function <void(int16*, const int16*)>Scan, std::function <void(int16*, const int16*)>InvScan)
{
  constexpr int32 NumIters = 32;
  constexpr int32 NumBlock = 1024 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);
  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Tmp = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Dst = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xTestUtils::fillRandom((uint16*)Src, NOT_VALID, NumPels, 1, 12);

  tDuration FS = (tDuration)0;
  tDuration IS = (tDuration)0;

  //warmup
  for(int32 i = 0; i < NumPels; i += BA) { Scan   (Tmp + i, Src + i); }
  for(int32 i = 0; i < NumPels; i += BA) { InvScan(Dst + i, Tmp + i); }
  CHECK(xTestUtils::isSameBuffer(Dst, Src, NumPels, true));

  //measure
  for(int32 j = 0; j < NumIters; j++)
  {
    tTimePoint T0 = tClock::now();
    for(int32 i = 0; i < NumPels; i += BA) { Scan   (Tmp + i, Src + i); }
    tTimePoint T1 = tClock::now();
    for(int32 i = 0; i < NumPels; i += BA) { InvScan(Dst + i, Tmp + i); }
    tTimePoint T2 = tClock::now();
    CHECK(xTestUtils::isSameBuffer(Dst, Src, NumPels, true));
    FS += T1 - T0;
    IS += T2 - T1;
  }

  int64 NumBytes      = BuffSize * NumIters;
  flt64 BytesPerSecFS = NumBytes / std::chrono::duration_cast<tDurationS>(FS).count();
  flt64 BytesPerSecIS = NumBytes / std::chrono::duration_cast<tDurationS>(IS).count();

  xMemory::xAlignedFree(Src);
  xMemory::xAlignedFree(Tmp);
  xMemory::xAlignedFree(Dst);

  return { BytesPerSecFS, BytesPerSecIS };
}

//===============================================================================================================================================================================================================

TEST_CASE("xScanSTD")
{
  testScan(xScanSTD::Scan, xScanSTD::InvScan);
}

#if X_SIMD_CAN_USE_AVX512
TEST_CASE("xScanAVX512")
{
  testScan(xScanAVX512::Scan, xScanAVX512::InvScan);
}
#endif

//===============================================================================================================================================================================================================

#ifdef NDEBUG 

TEST_CASE("xScanSTD-perf")
{
  auto [FS, IS] = perfScan(xScanSTD::Scan, xScanSTD::InvScan);
  fmt::print("TIME(xScanSTD::Scan   ) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xScanSTD::InvScan) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}

#if X_SIMD_CAN_USE_AVX512
TEST_CASE("xScanAVX512-perf")
{
  auto [FS, IS] = perfScan(xScanAVX512::Scan, xScanAVX512::InvScan);
  fmt::print("TIME(xScanAVX512::Scan   ) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xScanAVX512::InvScan) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}
#endif

#endif //def NDEBUG

//===============================================================================================================================================================================================================


