/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "xBitstream.h"
#include "xMemory.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================
// xBitstream
//===============================================================================================================================================================================================================

xBitstream::xBitstream()
{
  m_ByteBuffer  = nullptr;
  m_TmpBuff     = 0;  
  m_ByteAligned = true;
}
xBitstream& xBitstream::operator= (xBitstream&& Src)
{
  bindByteBuffer(Src.unbindByteBuffer());
  m_TmpBuff = Src.m_TmpBuff;
  m_RemainigTmpBufferBits = Src.m_RemainigTmpBufferBits;
  m_ByteAligned = Src.m_ByteAligned;

  return *this;
} //move assignment operator

//===============================================================================================================================================================================================================
// xBitstreamReader
//===============================================================================================================================================================================================================

void xBitstreamReader::init()
{
  m_TmpBuff               = 0;
  m_RemainigTmpBufferBits = 0;
  m_ByteAligned           = true;
  assert(m_ByteBuffer);
  xFechEntireTmpFromByteBuffer();
}
void xBitstreamReader::uninit()
{
  xCheckAlignment();
  assert(m_ByteAligned);
  if(m_RemainigTmpBufferBits)
  {
    int32 RemainingBytes = m_RemainigTmpBufferBits >> 3;
    m_ByteBuffer->modifyRead(-RemainingBytes);
    m_RemainigTmpBufferBits = 0;
  }
}
uint32 xBitstreamReader::peekBit()
{
  if(m_RemainigTmpBufferBits == 0) { xFechEntireTmpFromByteBuffer(); }
  uint32 Bit = m_TmpBuff>>(c_TmpBuffBits - 1);
  return Bit;
}
uint32 xBitstreamReader::peekBits(uint32 NumberOfBitsToPeek)
{
  assert(NumberOfBitsToPeek <= c_TmpBuffBits && NumberOfBitsToPeek<=32);
  uint32 Bits = 0;
  if(NumberOfBitsToPeek > m_RemainigTmpBufferBits) { xFechSomeBytesFromByteBuffer(); }
  Bits = (uint32)(m_TmpBuff>>(c_TmpBuffBits - NumberOfBitsToPeek));
  return Bits;
}
uint32 xBitstreamReader::peekByte()
{
  if(m_ByteAligned && m_RemainigTmpBufferBits == 0) { return m_ByteBuffer->peekU8(); }
  else                                              { return peekBits(8);            }
}
void xBitstreamReader::skipBits(uint32 NumberOfBitsToSkip)
{
  assert(NumberOfBitsToSkip <= c_TmpBuffBits && NumberOfBitsToSkip <= 32);

  if(NumberOfBitsToSkip <= m_RemainigTmpBufferBits) //skipping within a temporal buffer
  {
    m_RemainigTmpBufferBits -=  NumberOfBitsToSkip;
    m_TmpBuff               <<= NumberOfBitsToSkip;
  }
  else //skipping across temporal buffer boundary
  {
    //skip remaining bits
    uint32 Delta = NumberOfBitsToSkip - m_RemainigTmpBufferBits;
    xFechEntireTmpFromByteBuffer();
    m_RemainigTmpBufferBits -=  Delta;
    m_TmpBuff               <<= Delta;
  }
}
uint32 xBitstreamReader::readBit()
{
  if(m_RemainigTmpBufferBits == 0) { xFechEntireTmpFromByteBuffer(); }

  uint32 Bit = m_TmpBuff>>(c_TmpBuffBits - 1);
  m_RemainigTmpBufferBits -= 1;
  m_TmpBuff <<= 1;

  return Bit;
}
uint32 xBitstreamReader::readBits(uint32 NumberOfBitsToRead)
{
  assert(NumberOfBitsToRead <= c_TmpBuffBits && NumberOfBitsToRead <= 32);
  uint32 Bits = 0;
 
  if(NumberOfBitsToRead <= m_RemainigTmpBufferBits) //reading within a temporal buffer
  {
    Bits = (uint32)(m_TmpBuff>>(c_TmpBuffBits-NumberOfBitsToRead));
    m_RemainigTmpBufferBits -= NumberOfBitsToRead;
    m_TmpBuff <<= NumberOfBitsToRead;
    //assert(getNumBitsLeft() >= 0);
  }
  else //reading across temporal buffer boundary
  {
    //read remaining bits
    uint32 Delta = NumberOfBitsToRead - m_RemainigTmpBufferBits;
    Bits = (uint32)(m_TmpBuff>>(c_TmpBuffBits-m_RemainigTmpBufferBits));
    Bits <<= Delta;

    xFechEntireTmpFromByteBuffer();

    //load bits
    Bits |= m_TmpBuff>>(c_TmpBuffBits-Delta);
    m_RemainigTmpBufferBits -= Delta;
    m_TmpBuff <<= Delta;
  }

  return Bits;
}
uint32 xBitstreamReader::readByte()
{
  assert(m_RemainigTmpBufferBits >= 8 || m_ByteBuffer->getDataSize() > 0);
  if(m_ByteAligned && m_RemainigTmpBufferBits == 0) { return m_ByteBuffer->extractU8(); }
  else                                              { return readBits(8);               }
}
uint32 xBitstreamReader::readAlign()
{
  uint32 Bits = 0;
  uint32 NumBitsUntilByteAligned = getNumBitsUntilByteAligned();
  if(NumBitsUntilByteAligned) { Bits = readBits(NumBitsUntilByteAligned); }
  m_ByteAligned = true;  
  return Bits; 
}
uint32 xBitstreamReader::xFechSomeBytesFromByteBuffer()
{
  uint32 DataSize = m_ByteBuffer->getDataSize();
  uint32 FechSize = xMin(DataSize, (c_TmpBuffBits - m_RemainigTmpBufferBits) >> 3);
  if(FechSize >= 4)
  {
    const uint32 Shift = c_TmpBuffBits - m_RemainigTmpBufferBits - 32;
    m_TmpBuff >>= Shift;
    m_TmpBuff |= m_ByteBuffer->extractU32_BE();
    m_TmpBuff <<= Shift;
    m_RemainigTmpBufferBits += 32;
    FechSize -= 4;
  }
  if(FechSize)
  {
    m_TmpBuff >>= (c_TmpBuffBits - m_RemainigTmpBufferBits);
    for(uint32 ByteIdx = 0; ByteIdx < FechSize; ByteIdx++)
    {
      m_TmpBuff <<= 8; m_TmpBuff |= m_ByteBuffer->extractU8();
    }
    m_TmpBuff <<= c_TmpBuffBits - (m_RemainigTmpBufferBits + (FechSize << 3));
    m_RemainigTmpBufferBits += FechSize << 3;
  }
  return FechSize;
}
uint32 xBitstreamReader::xFechEntireTmpFromByteBuffer()
{
  uint32 DataSize = m_ByteBuffer->getDataSize();
  uint32 FechSize = xMin(DataSize, c_TmpBuffBytes);
  if(FechSize == c_TmpBuffBytes)
  { 
    m_TmpBuff = (uint64)(m_ByteBuffer->extractU64_BE()); m_RemainigTmpBufferBits = c_TmpBuffBits;
  }
  else
  {
    m_TmpBuff = 0;    
    for(uint32 ByteIdx = 0; ByteIdx < FechSize; ByteIdx++)
    { 
      m_TmpBuff <<= 8; m_TmpBuff |= m_ByteBuffer->extractU8();
    }
    m_TmpBuff <<= c_TmpBuffBits - (FechSize << 3);
    m_RemainigTmpBufferBits = FechSize << 3;
  }
  return FechSize;
}
void xBitstreamReader::fetchFromStream(std::istream* InStream)
{
  assert(m_RemainigTmpBufferBits==0 && m_ByteAligned);
  m_ByteBuffer->read(InStream);
  init();
}
void xBitstreamReader::fetchFromStream(xStream* InStream)
{
  assert(m_RemainigTmpBufferBits==0 && m_ByteAligned);
  m_ByteBuffer->read(InStream);
  init();
}

//===============================================================================================================================================================================================================
// xBitstreamWriter
//===============================================================================================================================================================================================================

void xBitstreamWriter::init()
{
  m_TmpBuff = 0;
  m_RemainigTmpBufferBits = c_TmpBuffBits;
  m_ByteAligned = true;
  assert(m_ByteBuffer);
}
void xBitstreamWriter::uninit()
{
  xCheckAlignment();
  assert(m_ByteAligned);
  if(m_RemainigTmpBufferBits != c_TmpBuffBits) { xFlushEntireTmpToByteBuffer(); }
}
void xBitstreamWriter::writeBit(uint32 Bit)
{
  assert((Bit & 0x01) == Bit);
  if(m_RemainigTmpBufferBits == 0) { xFlushEntireTmpToByteBuffer(); }

  m_TmpBuff = (m_TmpBuff<<1) | Bit;
  m_RemainigTmpBufferBits -= 1;
}
void xBitstreamWriter::writeBits(uint32 Bits, uint32 NumberOfBitsToWrite)
{
  assert( NumberOfBitsToWrite <= c_TmpBuffBits );
  assert( (Bits & (0xFFFFFFFF >> (c_TmpBuffBits - NumberOfBitsToWrite))) == Bits);
  //Bits &= Mask;

  if(m_RemainigTmpBufferBits >= NumberOfBitsToWrite)
  {
    m_TmpBuff = (m_TmpBuff<<NumberOfBitsToWrite) | Bits;
    m_RemainigTmpBufferBits -= NumberOfBitsToWrite;
  }
  else
  {
    uint32 Delta = NumberOfBitsToWrite - m_RemainigTmpBufferBits;
    m_TmpBuff = (m_TmpBuff<<m_RemainigTmpBufferBits) | (Bits >> Delta);
    m_RemainigTmpBufferBits = 0;

    xFlushEntireTmpToByteBuffer();

    //uint32 Mask = (0xFFFFFFFF >> (c_TmpBuffBits - NumberOfBitsToWrite));
    //Mask >>= m_RemainigTmpBufferBits;
    m_TmpBuff = Bits; // (Bits & Mask);
    m_RemainigTmpBufferBits = c_TmpBuffBits - Delta;
  }  
}
void xBitstreamWriter::writeByte(uint32 Byte)
{
  assert((Byte & 0xFF) == Byte);
  if(m_ByteAligned && m_RemainigTmpBufferBits == c_TmpBuffBits) { return m_ByteBuffer->appendU8((uint8)Byte); }
  else                                                          { return writeBits(Byte, 8);                  }
}
uint32 xBitstreamWriter::writeAlign(uint32 Bit)
{
  assert(Bit==0 || Bit==1);
  uint32 NumBitsUntilByteAligned = getNumBitsUntilByteAligned();
  if(NumBitsUntilByteAligned)
  { 
    uint32 Bits = Bit == 0 ? 0 : (1<<NumBitsUntilByteAligned)-1;
    writeBits(Bits, NumBitsUntilByteAligned);
  }
  m_ByteAligned = true;
  return NumBitsUntilByteAligned;
}
uint32 xBitstreamWriter::writeBuffer(xByteBuffer* Buffer, uint32 NumAlignmentBits)
{
  const int32 NumBytesToCopyDirectly = NumAlignmentBits == 0 ? Buffer->getDataSize() : Buffer->getDataSize() - 1;
  for(int32 i = 0; i < NumBytesToCopyDirectly; i++)
  {
    uint8 Byte = Buffer->extractU8();
    writeByte(Byte);
  }
  if(NumAlignmentBits > 0)
  {
    uint8 LastByte = Buffer->extractU8();
    uint8 LastBits = LastByte >> NumAlignmentBits;
    writeBits(LastBits, 8 - NumAlignmentBits);
  }
  return (NumBytesToCopyDirectly<<3) + NumAlignmentBits;
}
uint32 xBitstreamWriter::xFlushEntireTmpToByteBuffer()
{
  uint32 FlushSize;
  if(m_RemainigTmpBufferBits == 0)
  {
    m_ByteBuffer->appendU64_BE(m_TmpBuff);
    FlushSize = c_TmpBuffBytes;
  }
  else
  {
    FlushSize = c_TmpBuffBytes - (m_RemainigTmpBufferBits >> 3);
    uint32 EmptyBytes = c_TmpBuffBytes - FlushSize;
    m_TmpBuff <<= (EmptyBytes << 3);
    for(uint32 ByteIdx = 0; ByteIdx < FlushSize; ByteIdx++)
    {
      m_ByteBuffer->appendU8((uint8)(m_TmpBuff >> (c_TmpBuffBits - 8))); m_TmpBuff <<= 8;
    }    
  }
  m_RemainigTmpBufferBits = c_TmpBuffBits;
  return FlushSize;
}
void xBitstreamWriter::flushToStream(std::ostream* OutStream)
{
  uninit();
  m_ByteBuffer->write(OutStream);
}
void xBitstreamWriter::flushToStream(xStream* OutStream)
{
  uninit();
  m_ByteBuffer->write(OutStream);
}

//===============================================================================================================================================================================================================

} //end of namespace AVLib