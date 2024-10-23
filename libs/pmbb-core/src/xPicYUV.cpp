/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#include "xPicYUV.h"
#include "xMemory.h"
#include "xPixelOps.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================
// xPicYUV 
//===============================================================================================================================================================================================================
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// xPicYUV - general functions
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xPicYUV::create(int32V2 Size, int32 BitDepth, eCrF ChromaFormat, int32 Margin)
{
  int32 NumCmps = (int32)ChromaFormat == 400 ? 1 : 3;

  xInit(Size, BitDepth, Margin, NumCmps, sizeof(uint16));
  m_ChromaFormat    = ChromaFormat;
  m_BuffCmpNumPels  = NOT_VALID;
  m_BuffCmpNumBytes = NOT_VALID;

  //luma
  if((int32)ChromaFormat >= 400) { m_CmpSizeShiftN[(int32)eCmp::LM] ={ 0, 0 }; }
  else { ChromaFormat = (eCrF)((int32)ChromaFormat + 400); }

  //chroma
  switch(ChromaFormat)
  {
    case eCrF::CF444: m_CmpSizeShiftN[(int32)eCmp::CB] = m_CmpSizeShiftN[(int32)eCmp::CR] ={ 0,  0  }; break;
    case eCrF::CF422: m_CmpSizeShiftN[(int32)eCmp::CB] = m_CmpSizeShiftN[(int32)eCmp::CR] ={ 1,  0  }; break;
    case eCrF::CF420: m_CmpSizeShiftN[(int32)eCmp::CB] = m_CmpSizeShiftN[(int32)eCmp::CR] ={ 1,  1  }; break;
    case eCrF::CF400: m_CmpSizeShiftN[(int32)eCmp::CB] = m_CmpSizeShiftN[(int32)eCmp::CR] ={ 31, 31 }; break;
    default: assert(0); break;
  }

  for(int32 c=0; c < m_NumCmps; c++)
  {
    m_BuffCmpNumPelsN [c] = (getWidth((eCmp)c) + (m_Margin << 1)) * (getHeight((eCmp)c) + (m_Margin << 1));
    m_BuffCmpNumBytesN[c] = m_BuffCmpNumPelsN[c] * sizeof(uint16);
    m_Stride          [c] = getWidth((eCmp)c) + (m_Margin << 1);
    m_Buffer          [c] = (uint16*)xMemory::xAlignedMallocPageAuto(m_BuffCmpNumBytesN[c]);
    m_Origin          [c] = m_Buffer[c] + (m_Margin * m_Stride[c]) + m_Margin;
  }  
}
void xPicYUV::destroy()
{
  m_ChromaFormat = eCrF::INVALID;

  for(int32 c = 0; c < c_MaxNumCmps; c++)
  {
    m_CmpSizeShiftN   [c] = { NOT_VALID, NOT_VALID };
    m_BuffCmpNumPelsN [c] = NOT_VALID;
    m_BuffCmpNumBytesN[c] = NOT_VALID;
    m_Stride          [c] = NOT_VALID;
    if(m_Buffer[c] != nullptr) { xMemory::xAlignedFree(m_Buffer[c]); m_Buffer[c] = nullptr; }
    m_Origin[c] = nullptr;
  }

  xUnInit();
}
void xPicYUV::clear()
{
  for(int32 c=0; c < m_NumCmps; c++) { memset(m_Buffer[c], 0, m_BuffCmpNumBytesN[c]); }
  m_POC              = NOT_VALID;
  m_Timestamp        = NOT_VALID;
  m_IsMarginExtended = false;
}
void xPicYUV::copy(const xPicYUV* Src)
{
  assert(Src!=nullptr && isCompatible(Src));
  for(int32 c=0; c < m_NumCmps; c++) { memcpy(m_Buffer[c], Src->m_Buffer[c], m_BuffCmpNumBytesN[c]); }
  m_IsMarginExtended = Src->m_IsMarginExtended;
}
void xPicYUV::fill(uint16 Value)
{
  for(int32 c = 0; c < m_NumCmps; c++) { fill(Value, (eCmp)c); }
  m_IsMarginExtended = true;
}
void xPicYUV::fill(uint16 Value, eCmp CmpId)
{ 
  xPixelOps::Fill(m_Buffer[(int32)CmpId], Value, m_BuffCmpNumPelsN[(int32)CmpId]);
  m_IsMarginExtended = true;
}
bool xPicYUV::check(const std::string& Name) const
{
  boolV4 Correct = xMakeVec4(true);
  for(int32 c = 0; c < m_NumCmps; c++)
  { 
    Correct[c] = xPixelOps::CheckIfInRange(m_Origin[c], m_Stride[c], getWidth((eCmp)c), getHeight((eCmp)c), m_BitDepth);
  }

  for(int32 c = 0; c < m_NumCmps; c++)
  {
    if(!Correct[c])
    {
      fmt::print("FILE BROKEN " + Name + " (CMP={:d})\n", c);
      std::string Msg = xPixelOps::FindOutOfRange(m_Origin[c], getStride((eCmp)c), getWidth((eCmp)c), getHeight((eCmp)c), m_BitDepth, -1);
      fmt::print(Msg);
      return false;
    }
  }

  return true;
}
void xPicYUV::conceal()
{
  for(int32 c = 0; c < m_NumCmps; c++)
  {
    xPixelOps::ClipToRange(m_Origin[c], getStride((eCmp)c), getWidth((eCmp)c), getHeight((eCmp)c), m_BitDepth);
  }
  m_IsMarginExtended = false;
}
void xPicYUV::extend()
{
  for(int32 c = 0; c < m_NumCmps; c++) { xPixelOps::ExtendMargin(m_Origin[c], getStride((eCmp)c), getWidth((eCmp)c), getHeight((eCmp)c), m_Margin); }
  m_IsMarginExtended = true;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//low level buffer modification / access - dangerous
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool xPicYUV::bindBuffer(uint16* Buffer, eCmp CmpId)
{
  assert(Buffer!=nullptr); if(m_Buffer[(int32)CmpId]) { return false; }
  m_Buffer[(int32)CmpId] = Buffer;
  m_Origin[(int32)CmpId] = m_Buffer[(int32)CmpId] + (m_Margin * m_Stride[(int32)CmpId]) + m_Margin;
  return true;
}
uint16* xPicYUV::unbindBuffer(eCmp CmpId)
{
  if(m_Buffer[(int32)CmpId]==nullptr) { return nullptr; }
  uint16* Tmp = m_Buffer[(int32)CmpId];
  m_Buffer[(int32)CmpId] = nullptr;
  m_Origin[(int32)CmpId] = nullptr;
  return Tmp;
}
bool xPicYUV::swapBuffer(uint16*& Buffer, eCmp CmpId)
{
  assert(Buffer!=nullptr); if(m_Buffer[(int32)CmpId]==nullptr) { return false; }
  std::swap(m_Buffer[(int32)CmpId], Buffer);
  m_Origin[(int32)CmpId] = m_Buffer[(int32)CmpId] + (m_Margin * m_Stride[(int32)CmpId]) + m_Margin;
  return true;
}
bool xPicYUV::swapBuffer(xPicYUV* TheOther, eCmp CmpId)
{
  assert(TheOther != nullptr && isCmpCompatible(TheOther, CmpId)); if(TheOther==nullptr || !isCmpCompatible(TheOther, CmpId)) { return false; }
  std::swap(this->m_Buffer[(int32)CmpId], TheOther->m_Buffer[(int32)CmpId]);
  this    ->m_Origin[(int32)CmpId] = this    ->m_Buffer[(int32)CmpId] + (this    ->m_Margin * this    ->m_Stride[(int32)CmpId]) + this    ->m_Margin;
  TheOther->m_Origin[(int32)CmpId] = TheOther->m_Buffer[(int32)CmpId] + (TheOther->m_Margin * TheOther->m_Stride[(int32)CmpId]) + TheOther->m_Margin;
  return true;
}
bool xPicYUV::swapBuffers(xPicYUV* TheOther)
{
  for(int32 c = 0; c < m_NumCmps; c++)
  {
    bool Result = swapBuffer(TheOther, (eCmp)c);
    if(!Result) { return false; }
  }
  return true;
}

//===============================================================================================================================================================================================================

} //end of namespace PMBB
