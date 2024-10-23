/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_CodecCommon.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

void xCodecCommon::initCodecCommon(int32V2 PictureSize, eCrF ChromaFormat)
{
  m_PictureSize   = PictureSize;
  m_PictureWidth  = PictureSize.getX();
  m_PictureHeight = PictureSize.getY();
  m_ChromaFormat  = ChromaFormat;

  switch(m_ChromaFormat)
  {
    case eCrF::CF444: m_ProcessChroma = 1; m_NumOfComponents = 3; m_SampFactorHor = { 1, 1, 1, 0 }; m_SampFactorVer = { 1, 1, 1, 0 }; m_ShiftHor = { 0, 0, 0,32 }; m_ShiftVer = { 0, 0, 0,32 }; break;
    case eCrF::CF422: m_ProcessChroma = 1; m_NumOfComponents = 3; m_SampFactorHor = { 2, 1, 1, 0 }; m_SampFactorVer = { 1, 1, 1, 0 }; m_ShiftHor = { 0, 1, 1,32 }; m_ShiftVer = { 0, 0, 0,32 }; break;
    case eCrF::CF420: m_ProcessChroma = 1; m_NumOfComponents = 3; m_SampFactorHor = { 2, 1, 1, 0 }; m_SampFactorVer = { 2, 1, 1, 0 }; m_ShiftHor = { 0, 1, 1,32 }; m_ShiftVer = { 0, 1, 1,32 }; break;
    case eCrF::CF400: m_ProcessChroma = 0; m_NumOfComponents = 1; m_SampFactorHor = { 1, 0, 0, 0 }; m_SampFactorVer = { 1, 0, 0, 0 }; m_ShiftHor = { 0,32,32,32 }; m_ShiftVer = { 0,32,32,32 }; break;
    default: assert(0); break;
  }

  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    m_Log2MCUsWidth [CmpIdx] = c_L2BS + m_SampFactorHor[CmpIdx] - 1;
    m_Log2MCUsHeight[CmpIdx] = c_L2BS + m_SampFactorVer[CmpIdx] - 1;

    m_CmpWidth      [CmpIdx] = m_PictureWidth  >> m_ShiftHor[CmpIdx];
    m_CmpHeight     [CmpIdx] = m_PictureHeight >> m_ShiftVer[CmpIdx];

    m_MCUsMulWidth  [CmpIdx] = xRoundUpToNearestMultiple(m_CmpWidth [CmpIdx], m_Log2MCUsWidth [CmpIdx]);
    m_MCUsMulHeight [CmpIdx] = xRoundUpToNearestMultiple(m_CmpHeight[CmpIdx], m_Log2MCUsHeight[CmpIdx]);

    m_NumBlocks     [CmpIdx] = (m_MCUsMulWidth[CmpIdx] >> c_L2BS) * (m_MCUsMulHeight[CmpIdx] >> c_L2BS);

    m_ScanlineHeight[CmpIdx] = c_BS * m_SampFactorVer[CmpIdx];
  }

  m_NumMCUsInWidth  = xNumUnitsCoveringLength(m_PictureWidth,  m_Log2MCUsWidth [0]);
  m_NumMCUsInHeight = xNumUnitsCoveringLength(m_PictureHeight, m_Log2MCUsHeight[0]);
  m_NumMCUsInArea   = m_NumMCUsInWidth * m_NumMCUsInHeight;
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG