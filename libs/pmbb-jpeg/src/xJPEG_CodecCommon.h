/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_Constants.h"
#include "xVec.h"
#include "xJFIF.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xCodecCommon
{
protected:
  static constexpr int32 c_NC   = xJPEG_Constants::c_MaxComponents;
  static constexpr int32 c_L2BS = xJPEG_Constants::c_Log2BlockSize;
  static constexpr int32 c_BS   = xJPEG_Constants::c_BlockSize;
  static constexpr int32 c_BA   = xJPEG_Constants::c_BlockArea;

protected:
  int32V2 m_PictureSize     = { NOT_VALID, NOT_VALID };
  int32   m_PictureWidth    = NOT_VALID;
  int32   m_PictureHeight   = NOT_VALID;
  eCrF    m_ChromaFormat    = eCrF::INVALID;
  int32   m_NumOfComponents = NOT_VALID;
  int32   m_ProcessChroma   = false;

  std::array<int32, c_NC> m_SampFactorHor ;
  std::array<int32, c_NC> m_SampFactorVer ;
  std::array<int32, c_NC> m_ShiftHor      ;
  std::array<int32, c_NC> m_ShiftVer      ;
  std::array<int32, c_NC> m_Log2MCUsWidth ;
  std::array<int32, c_NC> m_Log2MCUsHeight;
  std::array<int32, c_NC> m_CmpWidth      ; //real size of image m_ExternalWidth
  std::array<int32, c_NC> m_CmpHeight     ;
  std::array<int32, c_NC> m_MCUsMulWidth  ; //size of image + padding (full MCU) m_BufferWidth
  std::array<int32, c_NC> m_MCUsMulHeight ;
  std::array<int32, c_NC> m_NumBlocks     ;
  std::array<int32, c_NC> m_ScanlineHeight;

  int32   m_NumMCUsInWidth  = NOT_VALID;
  int32   m_NumMCUsInHeight = NOT_VALID;
  int32   m_NumMCUsInArea   = NOT_VALID;
  int32   m_NumMCUsInSlice  = NOT_VALID;

  //operation
  int32   m_VerboseLevel    = NOT_VALID;

  //profiling
protected:
  tDuration  m_TotalPictureTime  = (tDuration)0;
  int64      m_TotalPictureIters = 0;
  int64      m_TotalSliceIters   = 0;
  bool       m_GatherTimeStats   = false;

public:
  void  setVerboseLevel(int32 VerboseLevel)       { m_VerboseLevel = VerboseLevel; }
  int32 getVerboseLevel(                  ) const { return m_VerboseLevel;         }

  void  setGatherTimeStats(bool GatherTimeStats)       { m_GatherTimeStats = GatherTimeStats; }
  bool  getGatherTimeStats(                    ) const { return m_GatherTimeStats;            }

protected:
  void initCodecCommon(int32V2 PictureSize, eCrF ChromaFormat);

  static inline void loadEntireBlock(uint16* restrict Dst, const uint16* Src, int32 SrcStride)
  {
    for(int32 y = 0; y < c_BS; y++)
    {
      ::memcpy(Dst, Src, c_BS * sizeof(uint16));
      Src += SrcStride; Dst += c_BS;
    }
  }

  static inline void storeEntireBlock(uint16* restrict Dst, const uint16* Src, int32 DstStride)
  {
    for(int32 y = 0; y < c_BS; y++)
    {
      ::memcpy(Dst, Src, c_BS * sizeof(uint16));
      Src += c_BS; Dst += DstStride;
    }
  }

  static inline void zeroEntireBlock(uint16* restrict Dst)
  {
    memset(Dst, 0, c_BA * sizeof(int16));
  }

  static inline void loadExtendBlock(uint16* restrict Dst, const uint16* Src, int32 SrcStride, int32 AvailableWidth, int32 AvailableHeight)
  {
    int32 y = 0;
    for(; y < AvailableHeight; y++)
    {
      int32 x = 0;
      for(; x < AvailableWidth; x++) { Dst[x] = Src[x]                 ; }
      for(; x < c_BS          ; x++) { Dst[x] = Dst[AvailableWidth - 1]; }
      Dst += c_BS;
      Src += SrcStride;
    }
    Src = Dst - c_BS;
    for(; y < c_BS; y++)
    {
      memcpy(Dst, Src, c_BS * sizeof(uint16));
      Dst += c_BS;
    }
  }

  static inline void storePartialBlock(uint16* restrict Dst, const uint16* Src, int32 DstStride, int32 AvailableWidth, int32 AvailableHeight)
  {    
    for(int32 y = 0; y < AvailableHeight; y++)
    {
      ::memcpy(Dst, Src, AvailableWidth * sizeof(uint16));
      Src += c_BS; Dst += DstStride;
    }
  }
};

//=====================================================================================================================================================================================

class xCodecImplCommon : public xCodecCommon
{
protected:
  //Markers
  xJFIF::xAPP0                    m_APP0;
  std::vector<xJFIF::xQuantTable> m_QT;
  int32                           m_RestartInterval = 0;
  xJFIF::xSOF0                    m_SOF0;
  std::vector<xJFIF::xHuffTable>  m_HT;
  xJFIF::xSOS                     m_SOS;
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG