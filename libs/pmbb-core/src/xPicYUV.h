/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once

#include "xCommonDefCORE.h"
#include "xPicCommon.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================
// xPicYUV - planar with different chroma formats allowed
//===============================================================================================================================================================================================================
class xPicYUV : public xPicCommon
{
protected:
  eCrF    m_ChromaFormat = eCrF::INVALID;
  int8V2  m_CmpSizeShiftN   [c_MaxNumCmps] = {{NOT_VALID, NOT_VALID}, {NOT_VALID, NOT_VALID}, {NOT_VALID, NOT_VALID}, {NOT_VALID, NOT_VALID}};
  int32   m_BuffCmpNumPelsN [c_MaxNumCmps] = { NOT_VALID, NOT_VALID, NOT_VALID, NOT_VALID };
  int32   m_BuffCmpNumBytesN[c_MaxNumCmps] = { NOT_VALID, NOT_VALID, NOT_VALID, NOT_VALID };
  int32   m_Stride          [c_MaxNumCmps] = { NOT_VALID, NOT_VALID, NOT_VALID, NOT_VALID };

  uint16* m_Buffer[c_MaxNumCmps] = { nullptr, nullptr, nullptr, nullptr }; //picture buffer
  uint16* m_Origin[c_MaxNumCmps] = { nullptr, nullptr, nullptr, nullptr }; //pel origin, pel access -> m_PelOrg[y*m_PelStride + x]

public:
  //constructors $ destructors
  xPicYUV() {};
  xPicYUV(int32V2 Size, int32 BitDepth, eCrF ChromaFormat, int32 Margin = c_DefMargin) { create(Size, BitDepth, ChromaFormat, Margin); }
  ~xPicYUV() { destroy(); }

  //genral functions
  void   create (int32V2 Size, int32 BitDepth, eCrF ChromaFormat, int32 Margin = c_DefMargin);
  void   create (const xPicYUV* Ref) { create(Ref->m_Size, Ref->m_BitDepth, Ref->m_ChromaFormat, Ref->m_Margin); }
  void   destroy();

  void   clear  (                              );
  void   copy   (const xPicYUV* Src            );
  void   copy   (const xPicYUV* Src, eCmp CmpId) { assert(isCompatible(Src)); xMemcpyX(m_Buffer[(int32)CmpId], Src->m_Buffer[(int32)CmpId], m_BuffCmpNumPelsN[(int32)CmpId]); }
  void   fill   (uint16 Value                  );
  void   fill   (uint16 Value      , eCmp CmpId);
  bool   check  (const std::string& Name       ) const;
  void   conceal(                              );
  void   extend (                              );

public:
  //inter-buffer compatibility functions
  inline bool    isSameSize      (int32V2 Size                  ) const { return (m_Width == Size.getX() && m_Height == Size.getY()); }
  inline bool    isSameSize      (int32V2 Size      , eCmp CmpId) const { return getSize(CmpId) == Size; }
  inline bool    isSameSize      (const xPicYUV* Pic, eCmp CmpId) const { return isSameSize(Pic->getSize(CmpId), CmpId); }

  //parameters
  inline eCrF    getChromaFormat(          ) const { return m_ChromaFormat; }
  inline int32   getSizeShiftHor(eCmp CmpId) const { return m_CmpSizeShiftN[(int32)CmpId].getX(); }
  inline int32   getSizeShiftVer(eCmp CmpId) const { return m_CmpSizeShiftN[(int32)CmpId].getY(); }
  inline int32V2 getSizeShift   (eCmp CmpId) const { return (int32V2)m_CmpSizeShiftN[(int32)CmpId]; }
  inline int32   getWidth       (eCmp CmpId) const { return m_Width  >> m_CmpSizeShiftN[(int32)CmpId].getX(); }
  inline int32   getHeight      (eCmp CmpId) const { return m_Height >> m_CmpSizeShiftN[(int32)CmpId].getY(); }
  inline int32V2 getSize        (eCmp CmpId) const { return {getWidth(CmpId), getHeight(CmpId)}; }
  inline int32   getArea        (eCmp CmpId) const { return getWidth(CmpId)*getHeight(CmpId); }

  //inter-buffer compatibility functions
  inline bool isSameChromaFmt(eCrF ChromaFormat) const { return m_ChromaFormat == ChromaFormat; }
  inline bool isCompatible(int32V2 Size, int32 BitDepth, eCrF ChromaFormat              ) const { return (isSameSize      (Size        ) && isSameBitDepth(BitDepth) && isSameChromaFmt(ChromaFormat)); }
  inline bool isCompatible(int32V2 Size, int32 BitDepth, eCrF ChromaFormat, int32 Margin) const { return (isSameSizeMargin(Size, Margin) && isSameBitDepth(BitDepth) && isSameChromaFmt(ChromaFormat)); }

  inline bool isSameChromaFmt(const xPicYUV* Pic) const { assert(Pic != nullptr); return isSameChromaFmt(Pic->m_ChromaFormat); }
  inline bool isCompatible   (const xPicYUV* Pic) const { return (isSameSizeMargin(Pic) && isSameBitDepth(Pic) && isSameChromaFmt(Pic)); }
  inline bool isCmpCompatible(const xPicYUV* Pic, eCmp CmpId) const { return (isSameSize(Pic, CmpId) && isSameMargin(Pic) && isSameBitDepth(Pic)); }

  //access picture data
  inline int32         getStride(                  eCmp CmpId) const { return m_Stride[(int32)CmpId]; }
  inline int32         getPitch (                            ) const { return 1                     ; }  
  inline uint16*       getAddr  (                  eCmp CmpId)       { return m_Origin[(int32)CmpId]; }
  inline const uint16* getAddr  (                  eCmp CmpId) const { return m_Origin[(int32)CmpId]; }
  inline int32         getOffset(int32V2 Position, eCmp CmpId) const { return Position.getY() * m_Stride[(int32)CmpId] + Position.getX(); }
  inline uint16*       getAddr  (int32V2 Position, eCmp CmpId)       { return getAddr(CmpId) + getOffset(Position, CmpId); }
  inline const uint16* getAddr  (int32V2 Position, eCmp CmpId) const { return getAddr(CmpId) + getOffset(Position, CmpId); }

  inline std::array<       int32 , c_MaxNumCmps> getStridesV() const { return { m_Stride[0], m_Stride[1], m_Stride[2], m_Stride[3] }; } //TODO C++20 - use to_array
  inline std::array<      uint16*, c_MaxNumCmps> getAddrsV  ()       { return { m_Origin[0], m_Origin[1], m_Origin[2], m_Origin[3] }; } //TODO C++20 - use to_array
  inline std::array<const uint16*, c_MaxNumCmps> getAddrsV  () const { return { m_Origin[0], m_Origin[1], m_Origin[2], m_Origin[3] }; } //TODO C++20 - use to_array

  //Buffer modification - dangerous         
  bool    bindBuffer  (uint16*  Buffer, eCmp CmpId);
  uint16* unbindBuffer(                 eCmp CmpId);
  bool    swapBuffer  (uint16*& Buffer, eCmp CmpId);
  bool    swapBuffer  (xPicYUV* TheOther, eCmp CmpId);
  bool    swapBuffers (xPicYUV* TheOther);
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB
