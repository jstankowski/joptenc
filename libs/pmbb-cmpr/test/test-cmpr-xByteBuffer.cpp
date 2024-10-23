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
#include <sstream>
#include "xTestUtils.h"
#include "xMemory.h"
#include "xCommonDefCMPR.h"
#include "xByteBuffer.h"

using namespace PMBB_NAMESPACE;

//===============================================================================================================================================================================================================

static constexpr int32 c_NumUnits = 1024*1024;

//===============================================================================================================================================================================================================

void testByteBufferU8()
{
  constexpr int32 DataSize   = sizeof(uint8) * c_NumUnits;
  constexpr int32 BufferSize = DataSize + 1;

  xByteBuffer ByteBuffer(BufferSize);
  std::stringstream Stream;

  //append
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    ByteBuffer.appendU8((uint8)State);
  }
  CHECK(ByteBuffer.getDataOffset() == 0); CHECK(ByteBuffer.getDataSize  () == DataSize);

  //write & read
  ByteBuffer.write(&Stream);
  ByteBuffer.clear();
  CHECK(ByteBuffer.getDataOffset() == 0); CHECK(ByteBuffer.getDataSize  () == 0);
  ByteBuffer.read(&Stream);
  CHECK(ByteBuffer.getDataOffset() == 0); CHECK(ByteBuffer.getDataSize  () == DataSize);

  //extract
  State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint8 Ref = (uint8)State;
    uint8 Rec = ByteBuffer.extractU8();
    CHECK(Rec == Ref);
  }
  CHECK(ByteBuffer.getDataOffset() == DataSize); CHECK(ByteBuffer.getDataSize  () == 0);
}

void testByteBufferU16()
{
  constexpr int32 DataSize   = sizeof(uint16) * c_NumUnits;
  constexpr int32 BufferSize = DataSize + 1;

  xByteBuffer ByteBufferBE(BufferSize);
  xByteBuffer ByteBufferLE(BufferSize);
  std::stringstream Stream;

  //append
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint16 Org = (uint16)State;
    ByteBufferBE.appendU16_BE(Org);
    ByteBufferLE.appendU16_LE(Org);
  }
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == DataSize);
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == DataSize);

  //write & read
  ByteBufferBE.write(&Stream);
  ByteBufferBE.clear();
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == 0);
  ByteBufferBE.read(&Stream);
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == DataSize);
  ByteBufferLE.write(&Stream);
  ByteBufferLE.clear();
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == 0);
  ByteBufferLE.read(&Stream);
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == DataSize);

  //extract
  State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint16 Ref = (uint16)State;
    uint16 RecBE = ByteBufferBE.extractU16_BE();
    uint16 RecLE = ByteBufferLE.extractU16_LE();
    CHECK(RecBE == Ref); CHECK(RecLE == Ref);
  }
  CHECK(ByteBufferBE.getDataOffset() == DataSize); CHECK(ByteBufferBE.getDataSize  () == 0);
  CHECK(ByteBufferLE.getDataOffset() == DataSize); CHECK(ByteBufferLE.getDataSize  () == 0);
}

void testByteBufferU32()
{
  constexpr int32 DataSize   = sizeof(uint32) * c_NumUnits;
  constexpr int32 BufferSize = DataSize + 1;

  xByteBuffer ByteBufferBE(BufferSize);
  xByteBuffer ByteBufferLE(BufferSize);
  std::stringstream Stream;

  //append
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint32 Org = (uint32)State;
    ByteBufferBE.appendU32_BE(Org);
    ByteBufferLE.appendU32_LE(Org);
  }
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == DataSize);
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == DataSize);

  //write & read
  ByteBufferBE.write(&Stream);
  ByteBufferBE.clear();
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == 0);
  ByteBufferBE.read(&Stream);
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == DataSize);
  ByteBufferLE.write(&Stream);
  ByteBufferLE.clear();
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == 0);
  ByteBufferLE.read(&Stream);
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == DataSize);

  //extract
  State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint32 Ref = (uint32)State;
    uint32 RecBE = ByteBufferBE.extractU32_BE();
    uint32 RecLE = ByteBufferLE.extractU32_LE();
    CHECK(RecBE == Ref); CHECK(RecLE == Ref);
  }
  CHECK(ByteBufferBE.getDataOffset() == DataSize); CHECK(ByteBufferBE.getDataSize  () == 0);
  CHECK(ByteBufferLE.getDataOffset() == DataSize); CHECK(ByteBufferLE.getDataSize  () == 0);
}

void testByteBufferU64()
{
  constexpr int32 DataSize   = sizeof(uint64) * c_NumUnits;
  constexpr int32 BufferSize = DataSize + 1;

  xByteBuffer ByteBufferBE(BufferSize);
  xByteBuffer ByteBufferLE(BufferSize);
  std::stringstream Stream;

  //append
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint64 Org = (((uint64)State)<<32) | (uint64)(xTestUtils::xXorShift32(State ^ 0xA3));
    ByteBufferBE.appendU64_BE(Org);
    ByteBufferLE.appendU64_LE(Org);
  }
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == DataSize);
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == DataSize);

  //write & read
  ByteBufferBE.write(&Stream);
  ByteBufferBE.clear();
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == 0);
  ByteBufferBE.read(&Stream);
  CHECK(ByteBufferBE.getDataOffset() == 0); CHECK(ByteBufferBE.getDataSize  () == DataSize);
  ByteBufferLE.write(&Stream);
  ByteBufferLE.clear();
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == 0);
  ByteBufferLE.read(&Stream);
  CHECK(ByteBufferLE.getDataOffset() == 0); CHECK(ByteBufferLE.getDataSize  () == DataSize);

  //extract


  State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint64 Ref = (((uint64)State) << 32) | (uint64)(xTestUtils::xXorShift32(State ^ 0xA3));
    uint64 RecBE = ByteBufferBE.extractU64_BE();
    uint64 RecLE = ByteBufferLE.extractU64_LE();
    CHECK(RecBE == Ref); CHECK(RecLE == Ref);
  }
  CHECK(ByteBufferBE.getDataOffset() == DataSize); CHECK(ByteBufferBE.getDataSize  () == 0);
  CHECK(ByteBufferLE.getDataOffset() == DataSize); CHECK(ByteBufferLE.getDataSize  () == 0);
}



//===============================================================================================================================================================================================================

TEST_CASE("U8")
{
  testByteBufferU8();
}

TEST_CASE("U16")
{
  testByteBufferU16();
}

TEST_CASE("U32")
{
  testByteBufferU32();
}

TEST_CASE("U64")
{
  testByteBufferU64();
}

