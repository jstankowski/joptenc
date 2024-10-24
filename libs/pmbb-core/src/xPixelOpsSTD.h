﻿/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefCORE.h"
#include "xPixelOpsBase.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================

class xPixelOpsSTD : public xPixelOpsBase
{
public:
  //Image
  static void  Cvt            (uint16* restrict Dst, const uint8*  Src, int32 DstStride, int32 SrcStride, int32 Width   , int32 Height   );
  static void  Cvt            (uint8*  restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 Width   , int32 Height   );
  static void  UpsampleHV     (uint16* restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  DownsampleHV   (uint16* restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  CvtUpsampleHV  (uint16* restrict Dst, const uint8*  Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  CvtDownsampleHV(uint8*  restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  UpsampleH      (uint16* restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  DownsampleH    (uint16* restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  CvtUpsampleH   (uint16* restrict Dst, const uint8*  Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static void  CvtDownsampleH (uint8*  restrict Dst, const uint16* Src, int32 DstStride, int32 SrcStride, int32 DstWidth, int32 DstHeight);
  static bool  CheckIfInRange (const uint16* Src, int32 Stride, int32 Width, int32 Height, int32 BitDepth);
  static tStr  FindOutOfRange (const uint16* Src, int32 Stride, int32 Width, int32 Height, int32 BitDepth, int32 MsgNumLimit);
  static void  ClipToRange    (uint16* restrict Ptr, int32 Stride, int32 Width, int32 Height, int32 BitDepth);
  static void  ExtendMargin   (uint16* Addr, int32 Stride, int32 Width, int32 Height, int32 Margin);
  static void  AOS4fromSOA3   (uint16* restrict DstABCD, const uint16* SrcA, const uint16* SrcB, const uint16* SrcC, uint16 ValueD, int32 DstStride, int32 SrcStride, int32 Width, int32 Height);
  static void  SOA3fromAOS4   (uint16* restrict DstA, uint16* restrict DstB, uint16* restrict DstC, const uint16* SrcABCD, int32 DstStride, int32 SrcStride, int32 Width, int32 Height);
  static int32 CountNonZero   (const uint16* Src, int32 SrcStride, int32 Width, int32 Height);
  static bool  CompareEqual   (const uint16* Tst, const uint16* Ref, int32 TstStride, int32 RefStride, int32 Width, int32 Height);
  static tStr  FindDiscrepancy(const uint16* Tst, const uint16* Ref, int32 TstStride, int32 RefStride, int32 Width, int32 Height, int32 MsgNumLimit);
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB
