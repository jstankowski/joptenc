/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "xCommonDefCMPR.h"
#include "xStream.h"
#include <vector>
#include <mutex>

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================

class xByteBuffer
{
protected:
  byte*    m_Buffer = nullptr;

  int32    m_BufferSize = NOT_VALID;
  int32    m_DataSize   = NOT_VALID;
  int32    m_DataOffset = NOT_VALID;
  
  uint64   m_EightCC   = 0;
  int64    m_POC       = std::numeric_limits<int64>::min();
  int64    m_Timestamp = std::numeric_limits<int64>::min();

  bool     m_OwnsBuffer;
  
public:
  //base functions
  xByteBuffer               ();
  xByteBuffer               (int32 BufferSize) { create(BufferSize); }
  xByteBuffer               (xByteBuffer* RefBuffer) { create(RefBuffer); }
  xByteBuffer               (byte* Memmory, int32 BufferSize                ) { m_BufferSize = BufferSize; m_DataSize = 0;        m_DataOffset = 0; m_OwnsBuffer = false; m_Buffer = Memmory; }
  xByteBuffer               (byte* Memmory, int32 BufferSize, int32 DataSize) { m_BufferSize = BufferSize; m_DataSize = DataSize; m_DataOffset = 0; m_OwnsBuffer = false; m_Buffer = Memmory; }
  ~xByteBuffer              () { destroy(); }

  void     create           (int32 BufferSize);
  void     create           (xByteBuffer* RefBuffer) { create(RefBuffer->getBufferSize()); }
  void     duplicate        (xByteBuffer* RefBuffer) { create(RefBuffer->getBufferSize()); copy(RefBuffer); }
  void     destroy          ();
  void     resize           (int32 BufferSize) { destroy(); create(BufferSize); }
  void     reset            () { m_DataSize = 0; m_DataOffset = 0; }
  void     clear            () { reset(); std::memset(m_Buffer, 0, m_BufferSize); }
  void     copy             (const xByteBuffer* Src);
  void     copy             (byte* Memmory, int32 Size);

  //tool functions
  inline bool   isSameSize       (int32 Size            ) const { return (m_BufferSize==Size); }
  inline bool   isSameSize       (xByteBuffer* RefBuffer) const { assert(RefBuffer!=nullptr); return isSameSize(RefBuffer->m_BufferSize); }

  //comparison functions
  inline bool   isSameDataSize   (xByteBuffer* RefBuffer) const { return (getDataSize() == RefBuffer->getDataSize()); }
  inline bool   isSameData       (xByteBuffer* RefBuffer) const { return ( isSameDataSize(RefBuffer) || std::memcmp(getReadPtr(), RefBuffer->getReadPtr(), getDataSize()) == 0); }

  //peek - read from beggining (pointed by DataOffset) without changing DataOffset (BE = Big Endian, LE = Little Endian)
  inline bool   peekBytes (byte* Dst, int32 Size) const;
  inline uint8  peekU8    () const { assert(m_DataSize >= 1); return             (*((uint8 *)(m_Buffer + m_DataOffset))); }
  inline uint16 peekU16_BE() const { assert(m_DataSize >= 2); return xSwapBytes16(*((uint16*)(m_Buffer + m_DataOffset))); }
  inline uint16 peekU16_LE() const { assert(m_DataSize >= 2); return             (*((uint16*)(m_Buffer + m_DataOffset))); }  
  inline uint32 peekU32_BE() const { assert(m_DataSize >= 4); return xSwapBytes32(*((uint32*)(m_Buffer + m_DataOffset))); }
  inline uint32 peekU32_LE() const { assert(m_DataSize >= 4); return             (*((uint32*)(m_Buffer + m_DataOffset))); }
  inline uint64 peekU64_BE() const { assert(m_DataSize >= 8); return xSwapBytes64(*((uint64*)(m_Buffer + m_DataOffset))); }
  inline uint64 peekU64_LE() const { assert(m_DataSize >= 8); return             (*((uint64*)(m_Buffer + m_DataOffset))); }
    
  //extract - read from beggining (pointed by DataOffset) with DataOffset update (BE = Big Endian, LE = Little Endian)
  bool     extractBytes (byte* Dst, int32 Size);
  uint8    extractU8    () { assert(m_DataSize >= 1); uint8  Data =             (*((uint8 *)(m_Buffer + m_DataOffset))); m_DataSize-- ; m_DataOffset++ ; return Data; }
  uint16   extractU16_BE() { assert(m_DataSize >= 2); uint16 Data = xSwapBytes16(*((uint16*)(m_Buffer + m_DataOffset))); m_DataSize-=2; m_DataOffset+=2; return Data; }
  uint16   extractU16_LE() { assert(m_DataSize >= 2); uint16 Data =             (*((uint16*)(m_Buffer + m_DataOffset))); m_DataSize-=2; m_DataOffset+=2; return Data; }
  uint32   extractU32_BE() { assert(m_DataSize >= 4); uint32 Data = xSwapBytes32(*((uint32*)(m_Buffer + m_DataOffset))); m_DataSize-=4; m_DataOffset+=4; return Data; }
  uint32   extractU32_LE() { assert(m_DataSize >= 4); uint32 Data =             (*((uint32*)(m_Buffer + m_DataOffset))); m_DataSize-=4; m_DataOffset+=4; return Data; }
  uint64   extractU64_BE() { assert(m_DataSize >= 8); uint64 Data = xSwapBytes64(*((uint64*)(m_Buffer + m_DataOffset))); m_DataSize-=8; m_DataOffset+=8; return Data; }
  uint64   extractU64_LE() { assert(m_DataSize >= 8); uint64 Data =             (*((uint64*)(m_Buffer + m_DataOffset))); m_DataSize-=8; m_DataOffset+=8; return Data; }

  //dispose - write at the end (pointed by DataOffset + DataSize) without changing DataSize (BE = Big Endian, LE = Little Endian)
  bool     disposeBytes (const byte* Src, int32 Size);
  bool     dispose      (xByteBuffer* Src) { return disposeBytes(Src->getReadPtr(), Src->getDataSize()); }
  void     disposeU8    (uint8  Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 1); *((uint8 *)(m_Buffer + m_DataOffset + m_DataSize)) = Data;               }
  void     disposeU16_BE(uint16 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 2); *((uint16*)(m_Buffer + m_DataOffset + m_DataSize)) = xSwapBytes16(Data); }
  void     disposeU16_LE(uint16 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 2); *((uint16*)(m_Buffer + m_DataOffset + m_DataSize)) =             (Data); }
  void     disposeU32_BE(uint32 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 4); *((uint32*)(m_Buffer + m_DataOffset + m_DataSize)) = xSwapBytes32(Data); }
  void     disposeU32_LE(uint32 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 4); *((uint32*)(m_Buffer + m_DataOffset + m_DataSize)) =             (Data); }
  void     disposeU64_BE(uint64 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 8); *((uint64*)(m_Buffer + m_DataOffset + m_DataSize)) = xSwapBytes64(Data); }
  void     disposeU64_LE(uint64 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 8); *((uint64*)(m_Buffer + m_DataOffset + m_DataSize)) =             (Data); }

  //append - write at the end (pointed by DataOffset + DataSize) with DataSize update (BE = Big Endian, LE = Little Endian)
  bool     appendBytes (const byte* Src, int32 Size);
  bool     append      (xByteBuffer* Src) { return appendBytes(Src->getReadPtr(), Src->getDataSize()); }
  void     appendU8    (uint8  Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 1); *((uint8 *)(m_Buffer + m_DataOffset + m_DataSize)) = Data;               m_DataSize++;  }
  void     appendU16_BE(uint16 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 2); *((uint16*)(m_Buffer + m_DataOffset + m_DataSize)) = xSwapBytes16(Data); m_DataSize+=2; }
  void     appendU16_LE(uint16 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 2); *((uint16*)(m_Buffer + m_DataOffset + m_DataSize)) = Data;               m_DataSize+=2; }
  void     appendU32_BE(uint32 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 4); *((uint32*)(m_Buffer + m_DataOffset + m_DataSize)) = xSwapBytes32(Data); m_DataSize+=4; }
  void     appendU32_LE(uint32 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 4); *((uint32*)(m_Buffer + m_DataOffset + m_DataSize)) = Data;               m_DataSize+=4; }
  void     appendU64_BE(uint64 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 8); *((uint64*)(m_Buffer + m_DataOffset + m_DataSize)) = xSwapBytes64(Data); m_DataSize+=8; }
  void     appendU64_LE(uint64 Data) { assert((m_BufferSize - (m_DataSize + m_DataOffset)) >= 8); *((uint64*)(m_Buffer + m_DataOffset + m_DataSize)) = Data;               m_DataSize+=8; }

  //transfer - read from beggining (pointed by DataOffset) with DataOffset update AND write at the end (pointed by DataOffset + DataSize) with DataSize update
  bool     transfer         (xByteBuffer* Src, int32 Size);
  bool     transfer         (xByteBuffer* Src) { return transfer(Src, Src->getDataSize()); }

  //align - moves data to begin of the buffer
  void     align            ();

  //read, write
  uint32   write(std::ostream* Stream) const;
  uint32   read (std::istream* Stream);
  uint32   write(xStream* Stream            ) const;
  uint32   read (xStream* Stream, int32 Size);
  uint32   read (xStream* Stream            ) { return read(Stream, m_BufferSize); }

  //interfaces
  byte*       getReadPtr       (                 )       { return m_Buffer + m_DataOffset;}
  const byte* getReadPtr       (                 ) const { return m_Buffer + m_DataOffset; }
  byte*       getWritePtr      (                 )       { return m_Buffer + m_DataOffset + m_DataSize; }
  int32       getDataSize      (                 ) const { return m_DataSize; }
  void        setDataSize      (int32 Size       )       { m_DataSize = Size; }
  void        modifyDataSize   (int32 DeltaSize  )       { m_DataSize += DeltaSize; }
  int32       getDataOffset    (                 ) const { return m_DataOffset;      }
  void        setDataOffset    (int32 DataOffset )       { m_DataOffset = DataOffset;}
  void        modifyDataOffset (int32 DeltaOffset)       { m_DataOffset += DeltaOffset; }
  void        modifyRead       (int32 NumBytes   )       { modifyDataOffset(NumBytes); modifyDataSize(-NumBytes); }
  void        modifyWritten    (int32 NumBytes   )       { modifyDataSize(NumBytes); }
  int32       getRemainingSize (                 ) const { return m_BufferSize - (m_DataSize + m_DataOffset); }
           
  int32       getBufferSize    (                 ) const { return m_BufferSize;     }
  byte*       getBufferPtr     (                 ) const { return m_Buffer;         }
                                                                           
  void        setEightCC       (uint64 EightCC   )       { m_EightCC = EightCC;     }
  uint64      getEightCC       (                 ) const { return m_EightCC;        }
  void        setPOC           (int64 POC        )       { m_POC = POC;             }
  int64       getPOC           (                 ) const { return m_POC;            }
  void        setTimestamp     (int64 Timestamp  )       { m_Timestamp = Timestamp; }
  int64       getTimestamp     (                 ) const { return m_Timestamp;      }
  void        copyDescriptors  (xByteBuffer* Ref )       { m_EightCC = Ref->m_EightCC; m_POC = Ref->m_POC; m_Timestamp = Ref->m_Timestamp; }
};

//===============================================================================================================================================================================================================

class xByteBufferRental
{
protected:
  std::vector<xByteBuffer*> m_Buffer;
  std::mutex                m_Mutex;
  uintSize                  m_CreatedUnits;
  uintSize                  m_SizeLimit;

  //Unit creation parameters
  int32    m_BufferSize;

public:
  void         create         (int32 BufferSize,        uintSize InitSize);
  void         create         (xByteBuffer* ByteBuffer, uintSize InitSize) { create(ByteBuffer->getBufferSize(), InitSize); }
  void         recreate       (int32 BufferSize,        uintSize InitSize) { destroy(); create(BufferSize, InitSize); }
  void         recreate       (xByteBuffer* ByteBuffer, uintSize InitSize) { destroy(); create(ByteBuffer, InitSize); }
  void         destroy        () { while(!m_Buffer.empty()) { xDestroyUnit(); } }

  void         setSizeLimit   (uintSize SizeLimit) { m_SizeLimit = SizeLimit; while(m_Buffer.size() > SizeLimit) { xDestroyUnit(); } }
                              
  void         put            (xByteBuffer* ByteBuffer);
  xByteBuffer* get            ();
                              
  bool         isCompatible   (xByteBuffer* ByteBuffer) { assert(ByteBuffer != nullptr); return ByteBuffer->isSameSize(m_BufferSize); }

  uintSize     getLoad        () { return m_Buffer.size(); }
  uintSize     getCreatedUnits() { return m_CreatedUnits;  }

protected:
  void         xCreateNewUnit ();
  void         xDestroyUnit   ();
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB_NAMESPACE