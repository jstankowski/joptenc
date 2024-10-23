/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "xByteBuffer.h"
#include "xMemory.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================
// xByteBuffer
//===============================================================================================================================================================================================================
xByteBuffer::xByteBuffer()
{
  m_BufferSize = NOT_VALID;
  m_DataSize   = NOT_VALID;
  m_DataOffset = NOT_VALID;
  m_OwnsBuffer = false;
  m_Buffer     = nullptr;  
  m_POC        = NOT_VALID;
  m_Timestamp  = NOT_VALID;
}
void xByteBuffer::create(int32 BufferSize)
{ 
  m_BufferSize = BufferSize;
  m_DataSize   = 0;
  m_DataOffset = 0;
  m_OwnsBuffer = true;
  m_Buffer     = (byte*)xMemory::xAlignedMallocPageAuto(m_BufferSize);
}
void xByteBuffer::destroy()
{
  m_BufferSize = NOT_VALID;
  m_DataSize   = NOT_VALID;
  m_DataOffset = NOT_VALID;
  if(m_Buffer != nullptr && m_OwnsBuffer) { xMemory::xAlignedFree(m_Buffer); }
  m_Buffer     = nullptr;
}
void xByteBuffer::copy(const xByteBuffer* Src)
{
  assert(Src->m_DataSize <= m_BufferSize);
  m_DataSize   = Src->m_DataSize;
  m_DataOffset = 0;
  std::memcpy(m_Buffer, Src->getReadPtr(), Src->getDataSize());
}
void xByteBuffer::copy(byte* Memmory, int32 Size)
{
  assert(Size <= m_BufferSize);
  m_DataSize   = Size;
  m_DataOffset = 0;
  std::memcpy(m_Buffer, Memmory, Size);
}
bool xByteBuffer::peekBytes(byte* Dst, int32 Size) const
{
  if(m_DataSize < Size) { return false; }
  std::memcpy(Dst, m_Buffer + m_DataOffset, Size);
  return true;
}
bool xByteBuffer::extractBytes(byte* Dst, int32 Size)
{
  if(m_DataSize < Size) { return false; }
  std::memcpy(Dst, m_Buffer + m_DataOffset, Size);
  m_DataOffset += Size;
  m_DataSize   -= Size;
  return true;
}
bool xByteBuffer::disposeBytes(const byte* Src, int32 Size)
{
  if(getRemainingSize() < Size) { return false; }
  std::memcpy(m_Buffer + m_DataOffset + m_DataSize, Src, Size);
  return true;
}
bool xByteBuffer::appendBytes(const byte* Src, int32 Size)
{
  if(getRemainingSize() < Size) { return false; }
  std::memcpy(m_Buffer + m_DataOffset + m_DataSize, Src, Size);
  m_DataSize += Size;
  return true;
}
bool xByteBuffer::transfer(xByteBuffer* Src, int32 Size)
{
  if((this->getRemainingSize() < Size) || Src->getDataSize() < Size) { return false; }
  std::memcpy(this->getWritePtr(), Src->getReadPtr(), Size);
  this->modifyWritten(Size);
  Src ->modifyRead(Size);
  return true;
}
void xByteBuffer::align()
{
  std::memmove(m_Buffer, getReadPtr(), m_DataSize);
  m_DataOffset = 0;
}
uint32 xByteBuffer::write(std::ostream* Stream) const
{
  assert(Stream && Stream->good());
  Stream->write((const char*)m_Buffer+m_DataOffset, m_DataSize);
  return m_DataSize;
}
uint32 xByteBuffer::read(std::istream* Stream)
{
  assert(Stream && Stream->good());
  reset();

  std::streampos CurrPos = Stream->tellg();
  Stream->seekg(0, std::istream::end);
  std::streampos EndPos = (uint32)Stream->tellg();
  Stream->seekg(CurrPos, std::istream::beg);
  uint64 RemainingLen = EndPos - CurrPos;
  uint64 NumBytesToRead = xMin(RemainingLen, (uint64)m_BufferSize);
  Stream->read((char*)m_Buffer, NumBytesToRead);
  modifyWritten((uint32)NumBytesToRead);
  return (uint32)NumBytesToRead;
}
uint32 xByteBuffer::write(xStream* Stream) const 
{
  assert(Stream && Stream->isValid());
  Stream->write(m_Buffer+m_DataOffset, m_DataSize);
  return m_DataSize;
}
uint32 xByteBuffer::read(xStream* Stream, int32 Size)
{
  assert(Stream && Stream->isValid());
  reset();
  uint64 CurrPos        = Stream->tellR();
  uint64 FileSize       = Stream->sizeR();
  uint64 RemainingLen   = FileSize - CurrPos;
  uint64 NumBytesToRead = xMin(RemainingLen, (uint64)Size);
  Stream->read(m_Buffer, (uint32)NumBytesToRead);
  modifyWritten((uint32)NumBytesToRead);
  return (uint32)NumBytesToRead;
}

//===============================================================================================================================================================================================================
// xByteBufferRental
//===============================================================================================================================================================================================================
void xByteBufferRental::create(int32 BufferSize, uintSize InitSize)
{
  m_Mutex.lock();
  m_BufferSize   = BufferSize;   
  m_CreatedUnits = 0;
  m_SizeLimit    = uintSize_max;

  for(uintSize i=0; i<InitSize; i++) { xCreateNewUnit(); }
  m_Mutex.unlock();
}
void xByteBufferRental::put(xByteBuffer* ByteBuffer)
{
  m_Mutex.lock();
  assert(ByteBuffer != nullptr);
  assert(isCompatible(ByteBuffer));
  m_Buffer.push_back(ByteBuffer);
  if(m_Buffer.size() > m_SizeLimit) { xDestroyUnit(); }
  m_Mutex.unlock();
}
xByteBuffer* xByteBufferRental::get()
{
  m_Mutex.lock();
  if(m_Buffer.empty()) { xCreateNewUnit(); }
  xByteBuffer* ByteBuffer = m_Buffer.back();
  m_Buffer.pop_back();
  m_Mutex.unlock();
  ByteBuffer->reset();
  return ByteBuffer;
}
void xByteBufferRental::xCreateNewUnit()
{
  xByteBuffer* Tmp = new xByteBuffer;
  Tmp->create(m_BufferSize); 
  m_Buffer.push_back(Tmp);
  m_CreatedUnits++;
}
void xByteBufferRental::xDestroyUnit()
{
  if(!m_Buffer.empty())
  {
    xByteBuffer* Tmp = m_Buffer.back();
    m_Buffer.pop_back();
    delete Tmp;
  }
}

//===============================================================================================================================================================================================================

} //end of namespace PMBB_NAMESPACE