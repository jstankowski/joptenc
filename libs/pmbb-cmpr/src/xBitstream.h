/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once
#include "xCommonDefCMPR.h"
#include "xByteBuffer.h"
#include "xStream.h"
#include <ostream>
#include <istream>

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================

class xBitstream
{
protected:
  xByteBuffer* m_ByteBuffer;
  uint64       m_TmpBuff;
  uint32       m_RemainigTmpBufferBits;
  bool         m_ByteAligned;

protected:
  static const uint32 c_TmpBuffBytes = sizeof(m_TmpBuff);
  static const uint32 c_TmpBuffBits  = c_TmpBuffBytes<<3;

protected:
  inline void xCheckAlignment() { m_ByteAligned = ((m_RemainigTmpBufferBits & 0x07) == 0); }
  
public:
  xBitstream();
  //avoid bitstreams beeing copied or copy assigned
  xBitstream            (const xBitstream&) = delete; //delete copy constructor
  xBitstream& operator= (const xBitstream&) = delete; //delete copy assignement operator

  xBitstream& operator= (xBitstream&& Src); //move assignment operator

  xByteBuffer* getByteBuffer   (                       ) { return m_ByteBuffer; }
  bool         bindByteBuffer  (xByteBuffer* ByteBuffer) { if(m_ByteBuffer != nullptr) { return false  ; } else { m_ByteBuffer = ByteBuffer; return true; } }
  xByteBuffer* unbindByteBuffer(                       ) { if(m_ByteBuffer == nullptr) { return nullptr; } else { xByteBuffer* Tmp = m_ByteBuffer; m_ByteBuffer = nullptr; return Tmp; } }

  virtual void init() = 0;

  bool   getIsByteAligned()           { xCheckAlignment(); return m_ByteAligned; }   
  uint32 getNumBitsUntilByteAligned() { return (m_RemainigTmpBufferBits & 0x7); }
};

//===============================================================================================================================================================================================================

class xBitstreamReader : public xBitstream
{
public:
  xBitstreamReader() : xBitstream() { m_RemainigTmpBufferBits = 0; };

  void         init  ();
  void         uninit();
  void         clear ();

  uint32       peekBit  ();
  uint32       peekBits (uint32 Length);
  uint32       peekByte ();

  void         skipBits (uint32 Length);

  uint32       readBit  ();
  uint32       readBits (uint32 Length);
  uint32       readByte ();
  uint32       readAlign();

  void         fetchFromBuffer() { assert(m_RemainigTmpBufferBits == 0); xFechEntireTmpFromByteBuffer(); }
  void         fetchFromStream(std::istream* InStream);
  void         fetchFromStream(xStream*      InStream);

  int32        getNumBitsLeft() const { return (m_ByteBuffer->getDataSize()<<3) + m_RemainigTmpBufferBits; }

protected:
  uint32       xFechSomeBytesFromByteBuffer(); //adds some bytes to data remaining in buffer
  uint32       xFechEntireTmpFromByteBuffer(); //asumes byte buffer is empty
};

//===============================================================================================================================================================================================================

class xBitstreamWriter : public xBitstream
{
public:
  xBitstreamWriter() : xBitstream() { m_RemainigTmpBufferBits = c_TmpBuffBits; };

  void         init  ();
  void         uninit();
  void         clear ();

  void         writeBit  (uint32 Bit);
  void         writeBits (uint32 Bits, uint32 NumberOfBits);
  void         writeByte (uint32 Byte);  
  uint32       writeAlign(uint32 Bit );

  uint32       writeBuffer(xByteBuffer* Buffer, uint32 NumAlignmentBits);

  void         flushToBuffer() { xCheckAlignment(); assert(m_ByteAligned); if(m_RemainigTmpBufferBits) { xFlushEntireTmpToByteBuffer(); }; }
  void         flushToStream(std::ostream* OutStream);
  void         flushToStream(xStream*      OutStream);

  uint32       getWrittenBits    () const { return (m_ByteBuffer->getDataSize()<<3) + (c_TmpBuffBits - m_RemainigTmpBufferBits); }
  bool         getIsFluchedToBuff() const { return m_RemainigTmpBufferBits==32; } 

protected:
  uint32       xFlushEntireTmpToByteBuffer();
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB_NAMESPACE

