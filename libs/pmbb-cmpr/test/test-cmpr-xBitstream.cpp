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
#include "xBitstream.h"

using namespace PMBB_NAMESPACE;

//===============================================================================================================================================================================================================

static constexpr int32 c_NumUnits = 1024*1024;

//===============================================================================================================================================================================================================

void testBitStream()
{
  constexpr int32 DataSize   = 8 * c_NumUnits;
  constexpr int32 BufferSize = DataSize + 1;

  xByteBuffer ByteBuffer(BufferSize);

  xBitstreamWriter BitW;
  xBitstreamReader BitR;
  
  //append
  BitW.bindByteBuffer(&ByteBuffer);
  BitW.init();
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint32 DataA = (State & 0b00000000000000000000000000000001);
    uint32 DataB = (State & 0b00000000000000000000000000011110) >> 1;
    uint32 DataC = (State & 0b00000000000000000000000000100000) >> 5;
    uint32 DataD = (State & 0b00000000000000000011111111000000) >> 6;
    uint32 DataE = (State & 0b00000000000000011100000000000000) >> 14;
    uint32 DataF = (State & 0b00000001111111100000000000000000) >> 17;
    BitW.writeBit (DataA   );
    BitW.writeBits(DataB, 4);
    BitW.writeBit (DataC   );
    BitW.writeByte(DataD   );
    BitW.writeBits(DataE, 3);
    BitW.writeByte(DataF   );

    if((i & 0x2FF) == 0)
    {
      if(BitW.getIsByteAligned())
      {
        BitW.writeBit(1);
      }
      uint32 DataG = (State & 0b00000010000000000000000000000000) >> 25;
      BitW.writeAlign(DataG);
    }

    State = xTestUtils::xXorShift32(State);
    uint32 SizeR = (State & 0x1F);
    if(SizeR == 0) { SizeR = 1; }
    uint32 DataR = (State >> SizeR) & ((1<< SizeR)-1);
    BitW.writeBits(DataR, SizeR);

  }
  BitW.writeAlign(0);
  BitW.uninit();
  BitW.unbindByteBuffer();

  //extract
  BitR.bindByteBuffer(&ByteBuffer);
  BitR.init();
  State = xTestUtils::c_XorShiftSeed;
  for(int32 i = 0; i < c_NumUnits; i++)
  {
    State = xTestUtils::xXorShift32(State);
    uint32 DataA = (State & 0b00000000000000000000000000000001);
    uint32 DataB = (State & 0b00000000000000000000000000011110) >> 1;
    uint32 DataC = (State & 0b00000000000000000000000000100000) >> 5;
    uint32 DataD = (State & 0b00000000000000000011111111000000) >> 6;
    uint32 DataE = (State & 0b00000000000000011100000000000000) >> 14;
    uint32 DataF = (State & 0b00000001111111100000000000000000) >> 17;
    CHECK(DataA == BitR.readBit ( ));
    CHECK(DataB == BitR.readBits(4));
    CHECK(DataC == BitR.readBit ( ));
    CHECK(DataD == BitR.readByte( ));
    CHECK(DataE == BitR.readBits(3));
    CHECK(DataF == BitR.readByte( ));

    if((i & 0x2FF) == 0)
    {
      if(BitR.getIsByteAligned())
      {
        CHECK(1 == BitR.readBit());
      }
      uint32 DataG = (State & 0b00000010000000000000000000000000) >> 25;
      CHECK(DataG == BitR.readAlign());
    }

    State = xTestUtils::xXorShift32(State);
    uint32 SizeR = (State & 0x1F);
    if(SizeR == 0) { SizeR = 1; }
    uint32 DataR = (State >> SizeR) & ((1 << SizeR) - 1);
    CHECK(DataR == BitR.readBits(SizeR));
  }
  CHECK(BitR.readAlign() == 0);
  BitR.uninit();
  BitR.unbindByteBuffer();

  CHECK(ByteBuffer.getDataSize() == 0);
}

//===============================================================================================================================================================================================================

TEST_CASE("BitStream")
{
  testBitStream();
}
