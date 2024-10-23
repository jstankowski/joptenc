/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <functional>
#include <utility>
#include <array>
#include "xTestUtils.h"
#include "xMemory.h"
#include "xCommonDefJPEG.h"
#include "xJPEG_Quant.h"
#include "xJPEG_Constants.h"

using namespace PMBB_NAMESPACE;
using namespace PMBB_NAMESPACE::JPEG;

//===============================================================================================================================================================================================================

constexpr int32 BA = xJPEG_Constants::c_BlockArea;

template<class T, std::size_t N> std::array<T, N> constexpr xMakeArray(T V)
{
  std::array<T, N> tempArray{};
  for(auto& elem : tempArray) { elem = V; }
  return tempArray;
}

void fillRandom(int16* Dst, int32 Area, uint32 Seed = xTestUtils::c_XorShiftSeed)
{
  uint32 State = Seed;
  for(int32 i = 0; i < Area; i++)
  {
    State = xTestUtils::xXorShift32(State);
    Dst[i] = (int16)(State & 0xFFFF);
  }
}

class xQuantTest : public xQuantizer
{
protected:
  uint16 m_QuantFlt[64];

public:
  const uint16* getQuantFlt  () const { return m_QuantFlt  ; };
  const uint16* getQuantCoeff() const { return m_QuantCoeff; };
  const uint16* getReciprocal() const { return m_Reciprocal; };
  const uint16* getCorrection() const { return m_Correction; };
  const uint16* getScale     () const { return m_Scale     ; };
  const uint16* getShift     () const { return m_Shift     ; };

  void initQuantFlt()
  {
    for(int32 i = 0; i < 64; i++) { m_QuantFlt[i] = m_QuantCoeff[i] << 4; }
  }

public:
  static int32 ComputeReciprocal(uint16 Divisor, uint16& Reciprocal, uint16& Correction, uint16& Scale, uint16& Shift) { return xComputeReciprocal(Divisor, Reciprocal, Correction, Scale, Shift); }
};

//===============================================================================================================================================================================================================

void testQuantSTD()
{
  {
    constexpr std::array<int16, 65536> TmpSequence = [] { std::array<int16, 65536> R{}; for(int32 i = 0; i < 65536; i++) { R[i] = (int16)(i - 32768); } return R; }();

    std::array<int16, 65536> RefQ;
    std::array<int16, 65536> TstQ;

    std::array<int16, 65536> RefI;
    std::array<int16, 65536> TstI;

    for(uint16 Divisor = 1; Divisor < 256; Divisor++)
    {
      uint16 Reciprocal;
      uint16 Correction;
      uint16 Scale;
      uint16 Shift;
      xQuantTest::ComputeReciprocal(Divisor, Reciprocal, Correction, Scale, Shift);

      const auto ArrayDivisor    = xMakeArray<uint16, 64>(Divisor   );
      const auto ArrayReciprocal = xMakeArray<uint16, 64>(Reciprocal);
      const auto ArrayCorrection = xMakeArray<uint16, 64>(Correction);
      const auto ArrayScale      = xMakeArray<uint16, 64>(Scale     );
      const auto ArrayShift      = xMakeArray<uint16, 64>(Shift     );

      //for(int32 i = 0; i < 65536; i++)
      //{
      //  RefQ[i] = (int16)round((flt64)TmpSequence[i] / (flt64)(Divisor));
      //}

      for(int32 i = 0; i < 65536; i += 64)
      {
        xQuantFLT::QuantScale(RefQ.data() + i, TmpSequence.data() + i, ArrayDivisor.data(), nullptr, nullptr);
      }

      for(int32 i = 0; i < 65536; i += 64)
      {
        xQuantSTD::QuantScale(TstQ.data() + i, TmpSequence.data() + i, ArrayCorrection.data(), ArrayReciprocal.data(), ArrayShift.data());
      }

      CHECK(xTestUtils::isSameBuffer(TstQ.data(), RefQ.data(), 65536, true));

      //for(int32 i = 0; i < 65536; i++)
      //{
      //  RefI[i] = (int16)((int32)RefQ[i] * (int32)(Divisor));
      //}

      for(int32 i = 0; i < 65536; i += 64)
      {
        xQuantFLT::InvScale(RefI.data() + i, RefQ.data() + i, ArrayDivisor.data());
      }

      for(int32 i = 0; i < 65536; i += 64)
      {
        xQuantSTD::InvScale(TstI.data() + i, RefQ.data() + i, ArrayDivisor.data());
      }

      CHECK(xTestUtils::isSameBuffer(TstI.data(), RefI.data(), 65536, true));
    }
  }

  {
    xQuantTest Quantizer;
    std::array<int16, BA> Src;
    std::array<int16, BA> RefQ;
    std::array<int16, BA> TstQ;
    std::array<int16, BA> RefI;
    std::array<int16, BA> TstI;

    for(int32 Quality = 100; Quality >= 0; Quality--)
    {
      Quantizer.Init(eCmp::LM, Quality);
      for(int32 r = 0; r < 1024; r++)
      {
        fillRandom(Src.data(), BA);

        for(int32 i = 0; i < BA; i++) { RefQ[i] = (int16)round((flt64)Src[i] / (flt64)(Quantizer.getQuantCoeff()[i]<<4)); }
        xQuantSTD::QuantScale(TstQ.data(), Src.data(), Quantizer.getCorrection(), Quantizer.getReciprocal(), Quantizer.getShift());
        CHECK(xTestUtils::isSameBuffer(TstQ.data(), RefQ.data(), BA, true));
        
        for(int32 i = 0; i < BA; i++) { RefI[i] = (int16)((int32)RefQ[i] * (int32)(Quantizer.getQuantCoeff()[i])); }
        xQuantSTD::InvScale(TstI.data(), RefQ.data(), Quantizer.getQuantCoeff());
        CHECK(xTestUtils::isSameBuffer(TstI.data(), RefI.data(), BA, true));
      }
    }
  }
}

void testQuantSIMD(std::function <void(int16*, const int16*, const uint16*, const uint16*, const uint16*)>QuantScale, std::function <void(int16*, const int16*, const uint16*)>InvScale)
{
  xQuantTest Quantizer;
  std::array<int16, BA> Src;
  std::array<int16, BA> RefQ;
  std::array<int16, BA> TstQ;
  std::array<int16, BA> RefI;
  std::array<int16, BA> TstI;

  for(int32 Quality = 100; Quality >= 0; Quality--)
  {
    Quantizer.Init(eCmp::LM, Quality);
    for(int32 r = 0; r < 1024; r++)
    {
      fillRandom(Src.data(), BA);

      xQuantSTD::QuantScale(RefQ.data(), Src.data(), Quantizer.getCorrection(), Quantizer.getReciprocal(), Quantizer.getShift());
                 QuantScale(TstQ.data(), Src.data(), Quantizer.getCorrection(), Quantizer.getReciprocal(), Quantizer.getScale());
      CHECK(xTestUtils::isSameBuffer(TstQ.data(), RefQ.data(), BA, true));

      xQuantSTD::InvScale(RefI.data(), RefQ.data(), Quantizer.getQuantCoeff());
                 InvScale(TstI.data(), RefQ.data(), Quantizer.getQuantCoeff());
      CHECK(xTestUtils::isSameBuffer(TstI.data(), RefI.data(), BA, true));
    }
  }
}

std::tuple<flt64, flt64> perfQuant(std::function <void(int16*, const int16*, const uint16*, const uint16*, const uint16*)>QuantScale,
                                   std::function <void(int16*, const int16*, const uint16*)>InvScale,
                                   bool MultiplicationBased, bool SimpleFloat)
{
  xQuantTest Quantizer;
  constexpr int32 NumIters = 32;
  constexpr int32 NumBlock = 1024 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);
  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Tmp = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Dst = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Ref = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xTestUtils::fillRandom((uint16*)Src, NOT_VALID, NumPels, 1, 16);

  Quantizer.Init(eCmp::LM, 50);
  if(SimpleFloat) { Quantizer.initQuantFlt(); }
  //reference
  for(int32 i = 0; i < NumPels; i += BA) { xQuantSTD::QuantScale(Tmp + i, Src + i, Quantizer.getCorrection(), Quantizer.getReciprocal(), Quantizer.getShift()); }
  for(int32 i = 0; i < NumPels; i += BA) { xQuantSTD::InvScale  (Ref + i, Tmp + i, Quantizer.getQuantCoeff()); }

  tDuration QS = (tDuration)0;
  tDuration IS = (tDuration)0;

  const uint16* Arg0 = SimpleFloat ? Quantizer.getQuantFlt() : Quantizer.getCorrection();
  const uint16* Arg1 = Quantizer.getReciprocal();
  const uint16* Arg2 = MultiplicationBased ? Quantizer.getScale() : Quantizer.getShift();

  //warmup  
  for(int32 i = 0; i < NumPels; i += BA) { QuantScale(Tmp + i, Src + i, Arg0, Arg1, Arg2); }
  for(int32 i = 0; i < NumPels; i += BA) { InvScale  (Dst + i, Tmp + i, Quantizer.getQuantCoeff()); }
  CHECK(xTestUtils::isSameBuffer(Dst, Ref, NumPels, true));

  //measure
  for(int32 j = 0; j < NumIters; j++)
  {
    tTimePoint T0 = tClock::now();
    for(int32 i = 0; i < NumPels; i += BA) { QuantScale(Tmp + i, Src + i, Arg0, Arg1, Arg2); }
    tTimePoint T1 = tClock::now();
    for(int32 i = 0; i < NumPels; i += BA) { InvScale(Dst + i, Tmp + i, Quantizer.getQuantCoeff()); }
    tTimePoint T2 = tClock::now();
    CHECK(xTestUtils::isSameBuffer(Dst, Ref, NumPels, true));
    QS += T1 - T0;
    IS += T2 - T1;
  }

  int64 NumBytes      = BuffSize * NumIters;
  flt64 BytesPerSecFS = NumBytes / std::chrono::duration_cast<tDurationS>(QS).count();
  flt64 BytesPerSecIS = NumBytes / std::chrono::duration_cast<tDurationS>(IS).count();
 
  xMemory::xAlignedFree(Src);
  xMemory::xAlignedFree(Tmp);
  xMemory::xAlignedFree(Dst);
  xMemory::xAlignedFree(Ref);

  return { BytesPerSecFS, BytesPerSecIS };
}

//===============================================================================================================================================================================================================

TEST_CASE("xQuantSTD")
{
  testQuantSTD();
}

#if X_SIMD_CAN_USE_SSE
TEST_CASE("xQuantSSE")
{
  testQuantSIMD(xQuantSSE::QuantScale, xQuantSSE::InvScale);
}
#endif

#if X_SIMD_CAN_USE_AVX
TEST_CASE("xQuantAVX")
{
  testQuantSIMD(xQuantAVX::QuantScale, xQuantAVX::InvScale);
}
#endif

#if X_SIMD_CAN_USE_AVX512
TEST_CASE("xQuantAVX512")
{
  testQuantSIMD(xQuantAVX512::QuantScale, xQuantAVX512::InvScale);
}
#endif

//===============================================================================================================================================================================================================

#ifdef NDEBUG 

TEST_CASE("xQuantFLT-perf")
{
  auto [FS, IS] = perfQuant(xQuantFLT::QuantScale, xQuantFLT::InvScale, false, true);
  fmt::print("TIME(xQuantFLT::QuantScale) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xQuantFLT::InvScale  ) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}

TEST_CASE("xQuantSTD-perf")
{
  auto [FS, IS] = perfQuant(xQuantSTD::QuantScale, xQuantSTD::InvScale, false, false);
  fmt::print("TIME(xQuantSTD::QuantScale) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xQuantSTD::InvScale  ) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}

#if X_SIMD_CAN_USE_SSE
TEST_CASE("xQuantSSE-perf")
{
  auto [FS, IS] = perfQuant(xQuantSSE::QuantScale, xQuantSSE::InvScale, true, false);
  fmt::print("TIME(xQuantSSE::QuantScale) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xQuantSSE::InvScale  ) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}
#endif

#if X_SIMD_CAN_USE_AVX
TEST_CASE("xQuantAVX-perf")
{
  auto [FS, IS] = perfQuant(xQuantAVX::QuantScale, xQuantAVX::InvScale, true, false);
  fmt::print("TIME(xQuantAVX::QuantScale) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xQuantAVX::InvScale  ) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}
#endif

#if X_SIMD_CAN_USE_AVX512
TEST_CASE("xQuantAVX512-perf")
{
  auto [FS, IS] = perfQuant(xQuantAVX512::QuantScale, xQuantAVX512::InvScale, true, false);
  fmt::print("TIME(xQuantAVX512::QuantScale) = {:.2f} MiB/s\n", FS / (1024 * 1024));
  fmt::print("TIME(xQuantAVX512::InvScale  ) = {:.2f} MiB/s\n", IS / (1024 * 1024));
}
#endif

#endif //def NDEBUG

//===============================================================================================================================================================================================================





