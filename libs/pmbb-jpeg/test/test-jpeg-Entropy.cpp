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
#include "xJPEG_Entropy.h"

using namespace PMBB_NAMESPACE;
using namespace PMBB_NAMESPACE::JPEG;

//===============================================================================================================================================================================================================

static constexpr int32 BA             = xJPEG_Constants::c_BlockArea;
static constexpr uint8 c_TabCoeff[64] = {47,64,60,53,60,69,47,53,57,53,42,44,47,39,29,11,25,29,32,32,29,6,14,13,21,11,3,5,3,3,4,5,4,4,2,1,0,1,2,2,1,2,4,4,1,0,1,1,0,0,0,0,0,3,1,0,0,0,0,0,0,0,0,0};

static uint32 fillRandomTransformCoeffsBlock(int16* Dst, uint32 State)
{
  for(int32 i = 0; i < 64; i++)
  {
    State = xTestUtils::xXorShift32(State);

    int32 Coeff = c_TabCoeff[i];
    int32_t Sign = State & 0x01 ? -1 : 1;
    
    int32_t Add  = (State & 0xC0 >> 6) >= 3 ? (State & 0x06) >> 1 : 0;
    int32_t Zero = (State & 0xFF00 >> 8) >= 192 ? 0 : 1;
    int32_t Shft = (State & 0xFF00 >> 8) <= 64  ? 3 : 0;
    if(i == 0) { Sign = 1; Zero = 1; Shft = 0; }
    int32 CoeffR = Zero * Sign * ((Coeff >> Shft) + Add);
    Dst[i] = (int16)xClipS16(CoeffR);
  }

  return State;
}

class xEntEncTest : public xEntropyEncoder
{
public:
  const xBitstreamWriter* getBitstream() const { return &m_Bitstream; }
};

class xEntEncDefTest : public xEntropyEncoderDefault
{
public:
  const xBitstreamWriter* getBitstream() const { return &m_Bitstream; }
};

//===============================================================================================================================================================================================================

void testEntropy(bool UseDefault)
{
  constexpr int32 NumIters = 64;
  constexpr int32 NumBlock = 4 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);

  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Dst = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xByteBuffer EntropyBuffer;
  EntropyBuffer.resize(BuffSize * 2);
  xByteBuffer FinalBuffer;
  FinalBuffer.resize(BuffSize * 4);
  
  std::vector<xJFIF::xHuffTable> HT;
  HT.resize(4);
  HT[0].InitDefault(0, xJFIF::xHuffTable::eHuffClass::DC, eCmp::LM);
  HT[1].InitDefault(0, xJFIF::xHuffTable::eHuffClass::AC, eCmp::LM);
  HT[2].InitDefault(1, xJFIF::xHuffTable::eHuffClass::DC, eCmp::CB); //any chroma so use CB
  HT[3].InitDefault(1, xJFIF::xHuffTable::eHuffClass::AC, eCmp::CB);

  xEntropyEncoder EntropyEnc;
  EntropyEnc.Init(HT);

  xEntropyEncoderDefault EntropyEncDef;

  xEntropyDecoder EntropyDec;
  EntropyDec.Init(HT);

  uint32 State = xTestUtils::c_XorShiftSeed;

  for(int32 j = 0; j < NumIters; j++)
  {
    //generate
    for(int32 i = 0; i < NumBlock; i++) { State = fillRandomTransformCoeffsBlock(Src + (i * BA), State); }

    for(int32 c = 0; c <= 1; c++)
    {
      FinalBuffer.reset();

      //encode
      EntropyBuffer.reset();
      if(UseDefault)
      {
        EntropyEncDef.StartSlice(&EntropyBuffer);
        for(int32 i = 0; i < NumBlock; i++) { EntropyEncDef.EncodeBlock(Src + (i * BA), eCmp(c)); }
        EntropyEncDef.FinishSlice();
      }
      else
      {
        EntropyEnc.StartSlice(&EntropyBuffer);
        for(int32 i = 0; i < NumBlock; i++) { EntropyEnc.EncodeBlock(Src + (i * BA), eCmp(c), c, c); }
        EntropyEnc.FinishSlice();
      }

      //copy to output and add stuffing
      xJFIF::AddStuffing(&FinalBuffer, &EntropyBuffer);

      EntropyBuffer.reset();

      //copy from input and remove stuffing until next marker
      xJFIF::RemoveStuffing(&EntropyBuffer, &FinalBuffer);

      //decode
      EntropyDec.StartSlice(&EntropyBuffer);
      for(int32 i = 0; i < NumBlock; i++) { EntropyDec.DecodeBlock(Dst + (i * BA), eCmp(c), c, c); }
      EntropyDec.FinishSlice();

      //compare
      CHECK(xTestUtils::isSameBuffer(Dst, Src, NumPels, true));
    }
  }

  xMemory::xAlignedFree(Src);
  xMemory::xAlignedFree(Dst);
}

void testEntropyEstimator(bool UseDefault)
{
  constexpr int32 NumIters = 64;
  constexpr int32 NumBlock = 4 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);

  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xByteBuffer EntropyBuffer;
  EntropyBuffer.resize(BuffSize * 2);

  std::vector<xJFIF::xHuffTable> HT;
  HT.resize(4);
  HT[0].InitDefault(0, xJFIF::xHuffTable::eHuffClass::DC, eCmp::LM);
  HT[1].InitDefault(0, xJFIF::xHuffTable::eHuffClass::AC, eCmp::LM);
  HT[2].InitDefault(1, xJFIF::xHuffTable::eHuffClass::DC, eCmp::CB); //any chroma so use CB
  HT[3].InitDefault(1, xJFIF::xHuffTable::eHuffClass::AC, eCmp::CB);

  xEntEncTest              EntropyEnc   ; EntropyEnc.Init(HT);
  xEntEncDefTest           EntropyEncDef;
  xEntropyEstimator        EntropyEst   ; EntropyEst.Init(HT);
  xEntropyEstimatorDefault EntropyEstDef;

  uint32 State = xTestUtils::c_XorShiftSeed;

  for(int32 j = 0; j < NumIters; j++)
  {
    //generate
    for(int32 i = 0; i < NumBlock; i++) { State = fillRandomTransformCoeffsBlock(Src + (i * BA), State); }

    for(int32 c = 0; c <= 1; c++)
    {
      //encode
      int32 EncBits = 0;
      EntropyBuffer.reset();
      if(UseDefault)
      {
        EntropyEncDef.StartSlice(&EntropyBuffer);
        for(int32 i = 0; i < NumBlock; i++) { EntropyEncDef.EncodeBlock(Src + (i * BA), eCmp(c)); }
        EncBits += EntropyEncDef.getBitstream()->getWrittenBits();
        EntropyEncDef.FinishSlice();
      }
      else
      {
        EntropyEnc.StartSlice(&EntropyBuffer);
        for(int32 i = 0; i < NumBlock; i++) { EntropyEnc.EncodeBlock(Src + (i * BA), eCmp(c), c, c); }
        EncBits += EntropyEnc.getBitstream()->getWrittenBits();
        EntropyEnc.FinishSlice();
      }

      //stimate
      int32 EstBits = 0;
      if(UseDefault)
      {
        EntropyEstDef.StartSlice();
        for(int32 i = 0; i < NumBlock; i++) { EstBits += EntropyEstDef.EstimateBlock(Src + (i * BA), eCmp(c)); }
        //EntropyEstDef.FinishSlice();
      }
      else
      {
        EntropyEst.StartSlice();
        for(int32 i = 0; i < NumBlock; i++) { EstBits += EntropyEst.EstimateBlock(Src + (i * BA), eCmp(c), c, c); }
        //EntropyEst.FinishSlice();
      }

      //compare
      CHECK(EncBits == EstBits);
    }
  }

  xMemory::xAlignedFree(Src);
}

std::tuple<flt64, flt64> perfEntropy(bool UseDefault)
{
  constexpr int32 NumIters = 16;
  constexpr int32 NumBlock = 1024 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);

  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xByteBuffer EntropyBuffer;
  EntropyBuffer.resize(BuffSize * 2);

  std::vector<xJFIF::xHuffTable> HT;
  HT.resize(4);
  HT[0].InitDefault(0, xJFIF::xHuffTable::eHuffClass::DC, eCmp::LM);
  HT[1].InitDefault(0, xJFIF::xHuffTable::eHuffClass::AC, eCmp::LM);
  HT[2].InitDefault(1, xJFIF::xHuffTable::eHuffClass::DC, eCmp::CB); //any chroma so use CB
  HT[3].InitDefault(1, xJFIF::xHuffTable::eHuffClass::AC, eCmp::CB);

  xEntEncTest              EntropyEnc   ; EntropyEnc.Init(HT);
  xEntEncDefTest           EntropyEncDef;
  xEntropyEstimator        EntropyEst   ; EntropyEst.Init(HT);
  xEntropyEstimatorDefault EntropyEstDef;

  uint32 State = xTestUtils::c_XorShiftSeed;

  tDuration EN = (tDuration)0;
  tDuration ES = (tDuration)0;

  for(int32 j = 0; j < NumIters; j++)
  {
    //generate
    for(int32 i = 0; i < NumBlock; i++) { State = fillRandomTransformCoeffsBlock(Src + (i * BA), State); }

    for(int32 c = 0; c <= 0; c++)
    {
      //encode
      int32 EncBits = 0;
      EntropyBuffer.reset();
      if(UseDefault)
      {
        EntropyEncDef.StartSlice(&EntropyBuffer);
        tTimePoint T = tClock::now();
        for(int32 i = 0; i < NumBlock; i++) { EntropyEncDef.EncodeBlock(Src + (i * BA), eCmp(c)); }
        EN += tClock::now() - T;
        EncBits += EntropyEncDef.getBitstream()->getWrittenBits();
        EntropyEncDef.FinishSlice();
      }
      else
      {
        EntropyEnc.StartSlice(&EntropyBuffer);
        tTimePoint T = tClock::now();
        for(int32 i = 0; i < NumBlock; i++) { EntropyEnc.EncodeBlock(Src + (i * BA), eCmp(c), c, c); }
        EN += tClock::now() - T;
        EncBits += EntropyEnc.getBitstream()->getWrittenBits();
        EntropyEnc.FinishSlice();
      }

      //stimate
      int32 EstBits = 0;
      if(UseDefault)
      {
        EntropyEstDef.StartSlice();
        tTimePoint T = tClock::now();
        for(int32 i = 0; i < NumBlock; i++) { EstBits += EntropyEstDef.EstimateBlock(Src + (i * BA), eCmp(c)); }
        ES += tClock::now() - T;
        //EntropyEstDef.FinishSlice();
      }
      else
      {
        EntropyEst.StartSlice();
        tTimePoint T = tClock::now();
        for(int32 i = 0; i < NumBlock; i++) { EstBits += EntropyEst.EstimateBlock(Src + (i * BA), eCmp(c), c, c); }
        ES += tClock::now() - T;
        //EntropyEst.FinishSlice();
      }

      //compare
      CHECK(EncBits == EstBits);
    }
  }

  int64 TotalNumBlock = NumBlock * NumIters;
  flt64 BlocksPerSecEN = TotalNumBlock / std::chrono::duration_cast<tDurationS>(EN).count();
  flt64 BlocksPerSecES = TotalNumBlock / std::chrono::duration_cast<tDurationS>(ES).count();

  xMemory::xAlignedFree(Src);

  return { BlocksPerSecEN, BlocksPerSecES };
}

//===============================================================================================================================================================================================================

#ifdef NDEBUG 

TEST_CASE("testEntropy")
{
  testEntropy(false);
}

TEST_CASE("testEntropyDefault")
{
  testEntropy(true);
}

TEST_CASE("testEstimator")
{
  testEntropyEstimator(false);
}

TEST_CASE("testEstimatorDefault")
{
  testEntropyEstimator(true);
}

TEST_CASE("testEntropy-perf")
{
  auto [EN, ES] = perfEntropy(false);
  fmt::print("TIME(xEntropyEncoder  ) = {:.2f} kBlock/s\n", EN / (1024));
  fmt::print("TIME(xEntropyEstimator) = {:.2f} kBlock/s\n", ES / (1024));
}
TEST_CASE("testEntropyDefault-perf")
{
  auto [EN, ES] = perfEntropy(true);
  fmt::print("TIME(xEntropyDefaultEncoder  ) = {:.2f} kBlock/s\n", EN / (1024));
  fmt::print("TIME(xEntropyDefaultEstimator) = {:.2f} kBlock/s\n", ES / (1024));
}

#endif //def NDEBUG

//===============================================================================================================================================================================================================




















//===============================================================================================================================================================================================================
