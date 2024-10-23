/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

//===============================================================================================================================================================================================================

#include "xCommonDefJPEG.h"
#include "xCfgINI.h"
#include "xFile.h"
#include "xPicYUV.h"
#include "xSeqLST.h"
#include "xJPEG_CodecSimple.h"
#include "xJPEG_Encoder.h"
#include "xMiscUtilsCORE.h"

namespace PMBB_NAMESPACE::JPEG {

//===============================================================================================================================================================================================================

class xAppJPEG
{
public:
  static const std::string_view c_BannerString;
  static const std::string_view c_HelpString  ;

protected:
  xCfgINI::xParser m_CfgParser;
  std::string      m_ErrorLog;

public:
  //basic io
  std::string m_InputFile     ;  
  std::string m_OutputFile    ;  
  std::string m_ReconFile     ;
  eFileFmt    m_FileFormat    ;
  int32V2     m_PictureSize   ;
  eImgTp      m_PictureType   ;  
  int32       m_BitDepth      ;  
  eCrF        m_ChromaFormat  ;  
  flt64       m_FrameRate     ;  
  int32       m_StartFrame    ;  
  int32       m_NumberOfFrames;
  //jpeg-specific
  eImpl       m_Implementation ;
  int32       m_Quality        ;
  int32       m_RestartInterval;
  //rdoq-specific
  int32       m_OptimizeLuma     ;
  int32       m_OptimizeChroma   ;
  int32       m_ProcessZeroCoeffs;
  int32       m_NumBlockOptPasses;
  eQTLa       m_QuantTabLayout   ;
  //validation 
  eActn       m_InvalidPelActn  ;
  eActn       m_NameMismatchActn;
  //operation
  int32       m_CalkPSNR       ;
  int32       m_NumberOfThreads;
  int32       m_VerboseLevel   ;

  //derrived
  bool  m_FileFormatRGB = false;
  bool  m_Validate      = false;
  bool  m_WriteBit      = false;
  bool  m_WriteRecon    = false;
  bool  m_Decode        = false;
  bool  m_ReorderRGB    = false;
  bool  m_CvtClrSpc     = false;
  int32 m_PicMargin     = NOT_VALID;
  bool  m_PrintFrame    = false;
  bool  m_GatherTime    = false;
  bool  m_PrintDebug    = false;


protected:
  //processing data
  int32 m_NumFrames = 0;

  //sequences and buffers
  xSeqBase* m_SeqOrg    = nullptr;
  xPicP*    m_PicOrgRGB = nullptr;
  xPicYUV*  m_PicOrg444 = nullptr;
  xPicYUV*  m_PicOrg4XX = nullptr;
  xPicYUV*  m_PicRec4XX = nullptr;
  xPicYUV*  m_PicRec444 = nullptr;
  xPicP*    m_PicRecRGB = nullptr; 
  xSeqBase* m_SeqRec    = nullptr;

  xByteBuffer  m_OutBuffer;
  xStream      m_OutFile;

  //encoders and decoders
  JPEG::xEncoderSimple   m_EncoderSimple;
  JPEG::xDecoderSimple   m_DecoderSimple;
#if X_PMBB_HAS_JPEG_TURBO
  JPEG::xEncoderTurbo    m_EncoderTurbo;
  JPEG::xDecoderTurbo    m_DecoderTurbo;
#endif //X_PMBB_HAS_JPEG_TURBO
  JPEG::xAdvancedEncoder m_EncoderRDOQ;

  //data & stats
  std::vector<flt64V4> m_FramePSNR_YUV;
  std::vector<flt64V4> m_FramePSNR_RGB;
  std::vector<uint64 > m_FrameBits;

  flt64V4 m_AvgPSNR_YUV;
  flt64V4 m_AvgPSNR_RGB;
  flt64   m_AvgFrameBytes;
  flt64   m_Bitrate;
  flt64   m_BitsPerPixel;
  flt64   m_ComprRatio;

  tTimePoint m_ProcBegTime  = tTimePoint::min();
  tTimePoint m_ProcEndTime  = tTimePoint::min();
  uint64     m_ProcBegTicks = 0;
  uint64     m_ProcEndTicks = 0;

  uint64  m_Ticks_LoadOrg = 0;
  uint64  m_TicksValidate = 0;
  uint64  m_Ticks_RGB2YUV = 0;
  uint64  m_Ticks__Encode = 0;
  uint64  m_TicksWriteBit = 0;
  uint64  m_Ticks__Decode = 0;
  uint64  m_Ticks_YUV2RGB = 0;
  uint64  m_TicksWriteRec = 0;
  uint64  m_TicksCalcPSNR = 0;

  flt64   m_InvDurationDenominator = 0;
  
public:
  void        registerCmdParams   ();
  bool        loadConfiguration   (int argc, const char* argv[]);
  bool        readConfiguration   ();
  std::string formatConfiguration ();
  eAppRes     validateInputFiles  ();
  std::string formatWarnings      ();

  eAppRes     setupSeqAndBuffs ();
  eAppRes     ceaseSeqAndBuffs ();
  void        createProcessors ();
  eAppRes     processAllFrames ();

  void        reorderRGB    ();
  eAppRes     validateFrames();
  void        cvtRGBtoYCbCr ();
  void        cvtYCbCrToRGB ();

  flt64V4     calcPicPSNR(const xPicP* Tst, const xPicP* Ref            , bool AvoidInfPSNR);
  flt64       calcCmpPSNR(const xPicP* Tst, const xPicP* Ref, eCmp CmpId, bool AvoidInfPSNR);

  flt64V4     calcPicPSNR(const xPicYUV* Tst, const xPicYUV* Ref            , bool AvoidInfPSNR);
  flt64       calcCmpPSNR(const xPicYUV* Tst, const xPicYUV* Ref, eCmp CmpId, bool AvoidInfPSNR);

  std::string calibrateTimeStamp();
  void        combineFrameStats ();

  std::string formatResultsStdOut();


public:
  const std::string& getErrorLog() { return m_ErrorLog; }
  int32 getVerboseLevel() { return m_VerboseLevel; }
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB::JPEG