/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_CodecCommon.h"
#include "xJFIF.h"
#include "xPicYUV.h"
#include "xJPEG_Quant.h"
#include "xJPEG_Entropy.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xCodecSimple : public xCodecImplCommon
{
protected:
  int32   m_Quality;

protected:
  xQuantizerSet m_Quant;
  xByteBuffer   m_EntropyBuffer;

//Profiling
protected:
  uint64 m_TotalPictureTicks   = 0;
  uint64 m_TotalSliceTicks     = 0;
  uint64 m_TotalMCUsTicks      = 0;
  uint64 m_TotalTransformTicks = 0;
  uint64 m_TotalQuantTicks     = 0;
  uint64 m_TotalScanTicks      = 0;
  uint64 m_TotalEntropyTicks   = 0;
  uint64 m_TotalStuffingTicks  = 0;

protected:
  void     xCreate ();
  void     xDestroy();

public:
  std::string formatAndResetStats(const std::string Prefix);
};

//=============================================================================================================================================================================

class xEncoderSimple : public xCodecSimple
{
protected:
  //encoder behaviour
  bool    m_EmitAPP0      = true;
  bool    m_EmitQuantTabs = true;
  bool    m_EmitHuffTabs  = true;

protected:
  xEntropyEncoder        m_EntropyEnc;
  xEntropyEncoderDefault m_EntropyEncDefault;

public: 
  void   create () { xCreate (); }
  void   destroy() { xDestroy(); }       

  void   init  (int32V2 PictureSize, eCrF ChromaFormat, int32 Quality, int32 RestartInterval, bool EmitAPP0, bool EmitQuantTabs, bool EmitHuffmanTabs);
  void   encode(const xPicYUV* InputPicture, xByteBuffer* OutputBuffer);

protected:
  void   xEncodePicture(const xPicYUV* InputPicture, xByteBuffer* OutputBuffer);
  void   xEncodeSlice  (const xPicYUV* InputPicture, xByteBuffer* OutputBuffer, int32 MCU_IdxFirst, int32 MCU_IdxLast); //slice - a MCUs between begin, reset or end
  void   xEncodeMCU    (const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx);
  void   xEncodeBlock  (const uint16* SamplesOrg, eCmp CmpId);
};

//=============================================================================================================================================================================

class xDecoderSimple : public xCodecSimple
{
protected:
  xEntropyDecoder m_EntropyDec;

public: 
  void   create () { xCreate (); }
  void   destroy() { xDestroy(); }   

  void   init   (int32V2 PictureSize, eCrF ChromaFormat, int32 Quality, int32 RestartInterval);
  bool   init   (xByteBuffer* InputBuffer);
  void   decode (xByteBuffer* InputBuffer, xPicYUV* OutputPicture); //assumes same parameters as previous valid one - does not parse headers
  
protected:
  void   xDecodePicture(xByteBuffer* InputBuffer, xPicYUV* OutputPicture);
  void   xDecodeSlice  (xByteBuffer* InputBuffer, xPicYUV* OutputPicture, int32 MCU_IdxFirst, int32 MCU_IdxLast); //slice - a MCUs between begin, reset or end
  void   xDecodeMCU    (uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx);
  void   xDecodeBlock  (uint16* SamplesDec, eCmp CmpId);
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG