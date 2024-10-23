/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#include "xAppJOptEnc.h"
#include "xFmtScn.h"
#include "xPixelOps.h"
#include "xColorSpace.h"
#include "xDistortion.h"
#include "xMathUtils.h"
#include "xSeqLST.h"

#include <numeric>

namespace PMBB_NAMESPACE::JPEG {

//===============================================================================================================================================================================================================

const std::string_view xAppJPEG::c_BannerString =
R"PMBBRAWSTRING(
=============================================================================
JOptEnc software v1.0

Copyright (c) 2020-2024, Jakub Stankowski, All rights reserved.

Developed at Poznan University of Technology, Poznan, Poland
Authors: Jakub Stankowski, Tomasz Grajek

The algorithm used in this software is described in following paper:
T. Grajek, J. Stankowski, "Rate-Distortion Optimized Quantization in Motion JPEG",
30th International Conference in Central Europe on Computer Graphics, Visualization and Computer Vision : WSCG 2022,

=============================================================================

)PMBBRAWSTRING";

const std::string_view xAppJPEG::c_HelpString =
R"PMBBRAWSTRING(
============================================================================= 
JOptEnc software v1.0

 Cmd | ParamName        | Description

usage::general --------------------------------------------------------------
 -i    InputFile          Input RAW/PNG/BMP file path
 -o    OutputFile         Output JPEG/MJPEG file path [optional]
 -r    ReconFile          Reconstructed RAW/PNG/BMP file path [optional]
 -ff   FileFormat         Format of input sequence (optional, default=RAW) [RAW, PNG, BMP]
 -ps   PictureSize        Size of input sequences (WxH)
 -pw   PictureWidth       Width of input sequences 
 -ph   PictureHeight      Height of input sequences
 -pf   PictureFormat      Picture format as defined by FFMPEG pix_fmt i.e. yuv420p10le
 -pt   PictureType        YCbCr / RGB   (optional, relevant to RAW format, defalult YCbCr)
 -bd   BitDepth           Bit depth     (optional, relevant to RAW format, default 8) 
 -cf   ChromaFormat       Chroma format (optional, default 420) [420, 422, 444]
 -sf   StartFrame         Start frame   (optional, default 0) 
 -nf   NumberOfFrames     Number of frames to be processed (optional, default -1=all)
 -fr   FrameRate          Sequence framerate - used to calculate bitrate (default 1) [optional]

PictureSize parameter can be used interchangeably with PictureWidth and PictureHeight. If PictureSize parameter is present the PictureWidth and PictureHeight arguments are ignored.
PictureFormat parameter can be used interchangeably with PictureType, BitDepth and ChromaFormat. If PictureFormat parameter is present the PictureType, BitDepth and ChromaFormat arguments are ignored.

PictureFormat, PictureType, BitDepth and ChromaFormat is not necessary for PNG/BMP input format.

usage::jpeg-specific --------------------------------------------------------
 -q    Quality            JPEG Quality (Q, 0-100, 0=lowest quality, 100=highest quality)
 -ri   RestartInterval    Restart interval in number of MCUs [0=disabled] (optional, default 0) 

usage::rdoq-specific --------------------------------------------------------
 -rol  OptimizeLuma       Apply RDOQ to luma blocks   (default 1) [optional]
 -roc  OptimizeChroma     Apply RDOQ to chroma blocks (default 1) [optional]
 -rpz  ProcessZeroCoeffs  Try optimize coeffs initially quantized to 0 (default 0) [optional]
 -rnp  NumBlockOptPasses  Number of optimization passes over one block (default 1) [optional]
 -qtl  QuantTabLayout     Select quantization table layout used during encoding:
                          [0 = default (RFC2435), 1 = flat, 2 = semi-flat] (default 0) [optional]
usage::valiation ------------------------------------------------------------
 -ipa  InvalidPelActn     Select action taken if invalid pixel value is detected 
                          (optional, default=STOP) [SKIP = disable pixel value checking,
                          WARN = print warning and ignore, STOP = stop execution,
                          CNCL = try to conceal by clipping to bit depth range]
 -nma  NameMismatchActn   Select action taken if parameters derived from filename are different
                          than provided as input parameters. Checks resolution, bit depth
                          and chroma format. (optional, default=WARN) [SKIP = disable checking,
                          WARN = print warning and ignore, STOP = stop execution]

usage::software_operation ---------------------------------------------------
 -cp   CalkPSNR           Calculate PSNR for reconstructed picture (default 1) [optional]
 -v    VerboseLevel       Verbose level (optional, default=1)

 -c    "config.cfg"       External config file - in INI format (optional)

-----------------------------------------------------------------------------
VerboseLevel:
  0 = final (average) metric values only
  1 = 0 + configuration + detected frame numbers
  2 = 1 + argc/argv + frame level metric values
  3 = 2 + computing time (could slightly slow down computations)

Example:
JOptEnc -i "A.yuv" -pw 1920 -ph 1080 -o "A.mjpeg" -r "A.yuv" -q 80 -v 3
JOptEnc -i "A.png" -ps 512x384 -o "A.jpeg" -r "I01_rec.png" -ff PNG -q 90 -v 3 
JOptEnc -i "A.bmp" -ps 512x384 -o "A.jpeg" -r "I01_rec.bmp" -ff BMP -q 90 -v 3

==============================================================================================================
)PMBBRAWSTRING";

//===============================================================================================================================================================================================================

void xAppJPEG::registerCmdParams()
{
  //general
  m_CfgParser.addCmdParm("i"  , "InputFile"     , "", "InputFile"     );
  m_CfgParser.addCmdParm("o"  , "OutputFile"    , "", "OutputFile"    );
  m_CfgParser.addCmdParm("r"  , "ReconFile"     , "", "ReconFile"     );
  m_CfgParser.addCmdParm("ff" , "FileFormat"    , "", "FileFormat"    );
  m_CfgParser.addCmdParm("ps" , "PictureSize"   , "", "PictureSize"   );
  m_CfgParser.addCmdParm("pw" , "PictureWidth"  , "", "PictureWidth"  );
  m_CfgParser.addCmdParm("ph" , "PictureHeight" , "", "PictureHeight" );
  m_CfgParser.addCmdParm("pf" , "PictureFormat" , "", "PictureFormat" );
  m_CfgParser.addCmdParm("pt" , "PictureType"   , "", "PictureType"   );
  m_CfgParser.addCmdParm("bd" , "BitDepth"      , "", "BitDepth"      );
  m_CfgParser.addCmdParm("cf" , "ChromaFormat"  , "", "ChromaFormat"  );
  m_CfgParser.addCmdParm("sf" , "StartFrame"    , "", "StartFrame"    );
  m_CfgParser.addCmdParm("nf" , "NumberOfFrames", "", "NumberOfFrames");
  m_CfgParser.addCmdParm("fr" , "FrameRate"     , "", "FrameRate"     );
  //jpeg-specific
  m_CfgParser.addCmdParm("imp", "Implementation" , "", "Implementation" );
  m_CfgParser.addCmdParm("q"  , "Quality"        , "", "Quality"        );
  m_CfgParser.addCmdParm("ri" , "RestartInterval", "", "RestartInterval");  
  //rdoq-specific
  m_CfgParser.addCmdParm("rol", "OptimizeLuma"     , "", "OptimizeLuma"     );
  m_CfgParser.addCmdParm("roc", "OptimizeChroma"   , "", "OptimizeChroma"   );
  m_CfgParser.addCmdParm("rpz", "ProcessZeroCoeffs", "", "ProcessZeroCoeffs");
  m_CfgParser.addCmdParm("rnp", "NumBlockOptPasses", "", "NumBlockOptPasses");
  m_CfgParser.addCmdParm("qtl", "QuantTabLayout"   , "", "QuantTabLayout"   );
  //validation 
  m_CfgParser.addCmdParm("ipa", "InvalidPelActn"  , "", "InvalidPelActn"  );
  m_CfgParser.addCmdParm("nma", "NameMismatchActn", "", "NameMismatchActn");
  //operation
  m_CfgParser.addCmdParm("cp" , "CalkPSNR"        , "", "CalkPSNR"        );
  m_CfgParser.addCmdParm("nth", "NumberOfThreads" , "", "NumberOfThreads" );  
  m_CfgParser.addCmdParm("v"  , "VerboseLevel"    , "", "VerboseLevel"    );  
}
bool xAppJPEG::loadConfiguration(int argc, const char* argv[])
{
  bool CommandlineResult = m_CfgParser.loadFromCmdln(argc, argv);
  if(!CommandlineResult)
  {
    m_ErrorLog += "! invalid commandline\n";
    m_ErrorLog += m_CfgParser.getParsingLog();
  }
  return CommandlineResult;
}
bool xAppJPEG::readConfiguration()
{
  bool AnyError = false;

  //basic io ----------------------------------------------------------------------------------------------------------
  m_InputFile      = m_CfgParser.getParam1stArg("InputFile"     , std::string(""));
  if(m_InputFile.empty()) { m_ErrorLog += "!  InputFile is empty\n"; AnyError = true; }
  m_OutputFile     = m_CfgParser.getParam1stArg("OutputFile"    , std::string(""));
  m_ReconFile      = m_CfgParser.getParam1stArg("ReconFile"     , std::string(""));

  m_FileFormat    = m_CfgParser.cvtParam1stArg("FileFormat", eFileFmt::RAW, xStr2FileFmt);
  if(m_FileFormat == eFileFmt::INVALID) { m_ErrorLog += "!  FileFormat is invalid\n"; AnyError = true; }
  m_FileFormatRGB = m_FileFormat == eFileFmt::BMP || m_FileFormat == eFileFmt::PNG;

  if(m_CfgParser.findParam("PictureSize"))
  {
    std::string PictureSizeS = m_CfgParser.getParam1stArg("PictureSize", std::string(""));
    m_PictureSize = xFmtScn::scanResolution(PictureSizeS);
    if(m_PictureSize[0] <= 0 || m_PictureSize[1] <= 0) { m_ErrorLog += "!  Invalid PictureSize value\n"; AnyError = true; }
  }
  else
  {
    int32 PictureWidth  = m_CfgParser.getParam1stArg("PictureWidth" , NOT_VALID);
    int32 PictureHeight = m_CfgParser.getParam1stArg("PictureHeight", NOT_VALID);
    m_PictureSize.set(PictureWidth, PictureHeight);
    if(PictureWidth  <= 0) { m_ErrorLog += "!  Invalid PictureWidth value\n" ; AnyError = true; }
    if(PictureHeight <= 0) { m_ErrorLog += "!  Invalid PictureHeight value\n"; AnyError = true; }
  }

  if(m_CfgParser.findParam("PictureFormat"))
  {
    std::string PictureFormatS = m_CfgParser.getParam1stArg("PictureFormat", std::string(""));
    eImgTp ImageType;
    std::tie(ImageType, m_ChromaFormat, m_BitDepth) = xFmtScn::scanPixelFormat(PictureFormatS);
    if(ImageType != eImgTp::YCbCr       ) { m_ErrorLog += "!  Invalid or unsuported ImageType value derrived from PictureFormat\n"; AnyError = true; }
    if(m_BitDepth < 8 || m_BitDepth > 14) { m_ErrorLog += "!  Invalid or unsuported BitDepth value derrived from PictureFormat\n"; AnyError = true; }
    if(m_ChromaFormat == eCrF::INVALID  ) { m_ErrorLog += "!  Invalid or unsuported ChromaFormat value derrived from PictureFormat\n"; AnyError = true; }
  }
  else
  {
    m_PictureType  = m_CfgParser.cvtParam1stArg("PictureType"   , m_FileFormatRGB ? eImgTp::RGB : eImgTp::YCbCr, xStr2ImgTp);
    m_BitDepth     = m_CfgParser.getParam1stArg("BitDepth"      , 8);  
    m_ChromaFormat = m_CfgParser.cvtParam1stArg("ChromaFormat"  , eCrF::CF420, xStr2CrF  );
    if(m_PictureType != eImgTp::YCbCr && m_PictureType != eImgTp::RGB) { m_ErrorLog += "!  Invalid or unsuported PictureType value\n"; AnyError = true; }
    if(m_BitDepth < 8 || m_BitDepth > 14) { m_ErrorLog += "!  Invalid or unsuported BitDepth value\n"    ; AnyError = true; }
    if(m_ChromaFormat == eCrF::INVALID  ) { m_ErrorLog += "!  Invalid or unsuported ChromaFormat value\n"; AnyError = true; }
  }

  m_StartFrame     = m_CfgParser.getParam1stArg("StartFrame"    , 0  );  
  m_NumberOfFrames = m_CfgParser.getParam1stArg("NumberOfFrames", -1 );  
  m_FrameRate      = m_CfgParser.getParam1stArg("FrameRate"     , 1.0);
  if(m_StartFrame < 0) { m_ErrorLog += "!  StartFrame value cannot be negative\n"; AnyError = true; }
  if(m_FrameRate <= 0) { m_ErrorLog += "!  FrameRate value have to be positive\n"; AnyError = true; }

  //jpeg-specific -----------------------------------------------------------------------------------------------------
  m_Implementation  = m_CfgParser.cvtParam1stArg("Implementation"  , eImpl::Advanded, xStrToImpl);
  m_Quality         = m_CfgParser.getParam1stArg("Quality"         , NOT_VALID);
  if(m_Quality < 0 || m_Quality > 100) { m_ErrorLog += "!  Quality value have to be in range [0-100]\n"; AnyError = true; }
  m_RestartInterval = m_CfgParser.getParam1stArg("RestartInterval" , 0  );
  
  //rdoq-specific -----------------------------------------------------------------------------------------------------
  m_OptimizeLuma      = m_CfgParser.getParam1stArg("OptimizeLuma"     , 1);
  m_OptimizeChroma    = m_CfgParser.getParam1stArg("OptimizeChroma"   , 1);
  m_ProcessZeroCoeffs = m_CfgParser.getParam1stArg("ProcessZeroCoeffs", 0);
  m_NumBlockOptPasses = m_CfgParser.getParam1stArg("NumBlockOptPasses", 1);
  m_QuantTabLayout    = m_CfgParser.cvtParam1stArg("QuantTabLayout"   , eQTLa::Default, xStrToQTLa);
  
  //validation --------------------------------------------------------------------------------------------------------
  std::string InvalidPelActnS = m_CfgParser.getParam1stArg("InvalidPelActn", "STOP");
  std::string NameMismatchActnS = m_CfgParser.getParam1stArg("NameMismatchActn", "WARN");
  m_InvalidPelActn = xStr2Actn(InvalidPelActnS);
  m_NameMismatchActn = xStr2Actn(NameMismatchActnS);

  //operation ---------------------------------------------------------------------------------------------------------
  m_CalkPSNR        = m_CfgParser.getParam1stArg("CalkPSNR"       , 1        );
  m_NumberOfThreads = m_CfgParser.getParam1stArg("NumberOfThreads", NOT_VALID);
  m_VerboseLevel    = m_CfgParser.getParam1stArg("VerboseLevel"   , 1        );

  //derrived ----------------------------------------------------------------------------------------------------------  
  m_Validate   = m_InvalidPelActn != eActn::SKIP;
  m_WriteBit   = !m_OutputFile.empty();
  m_WriteRecon = !m_ReconFile .empty();
  m_Decode     = m_WriteRecon || m_CalkPSNR;
  m_ReorderRGB = m_PictureType == eImgTp::BGR || m_PictureType == eImgTp::GBR;
  m_CvtClrSpc  = m_PictureType == eImgTp::RGB || m_PictureType == eImgTp::BGR || m_PictureType == eImgTp::GBR;
  m_PicMargin  = 8;
  m_PrintFrame = m_VerboseLevel >= 2;
  m_GatherTime = m_VerboseLevel >= 3;
  m_PrintDebug = m_VerboseLevel >= 4;

  return !AnyError;
}
std::string xAppJPEG::formatConfiguration()
{
  std::string Config; Config.reserve(xMemory::c_MemSizePageBase);
  //basic io
  Config += "Run-time configuration:\n";
  Config += fmt::format("InputFile         = {}\n", m_InputFile);
  Config += fmt::format("OutputFile        = {}\n", m_OutputFile.empty() ? "(unused)" : m_OutputFile);
  Config += fmt::format("ReconFile         = {}\n", m_ReconFile .empty() ? "(unused)" : m_ReconFile );
  Config += fmt::format("FileFormat        = {}\n", xFileFmt2Str(m_FileFormat));
  Config += fmt::format("PictureSize       = {}\n", xFmtScn::formatResolution(m_PictureSize));
  Config += fmt::format("PictureType       = {}\n", xImgTp2Str(m_PictureType));
  Config += fmt::format("BitDepth          = {}\n", m_BitDepth);
  Config += fmt::format("ChromaFormat      = {}\n", xCrF2Str(m_ChromaFormat));
  Config += fmt::format("FrameRate         = {}\n", m_FrameRate);
  Config += fmt::format("StartFrame        = {}\n", m_StartFrame);
  Config += fmt::format("NumberOfFrames    = {}{}\n", m_NumberOfFrames, m_NumberOfFrames == NOT_VALID ? "  (all)" : "");
  //jpeg-specific
  Config += fmt::format("Implementation    = {}\n", xImplToStr(m_Implementation));
  Config += fmt::format("Quality           = {}\n", m_Quality        );
  Config += fmt::format("RestartInterval   = {}\n", m_RestartInterval);
  //rdoq-specific
  Config += fmt::format("OptimizeLuma      = {}\n", m_OptimizeLuma     );
  Config += fmt::format("OptimizeChroma    = {}\n", m_OptimizeChroma   );
  Config += fmt::format("ProcessZeroCoeffs = {}\n", m_ProcessZeroCoeffs);
  Config += fmt::format("NumBlockOptPasses = {}\n", m_NumBlockOptPasses);
  Config += fmt::format("QuantTabLayout    = {}\n", xQTLaToStr(m_QuantTabLayout));
  //validation 
  Config += fmt::format("InvalidPelActn    = {}\n", xActn2Str(m_InvalidPelActn));
  Config += fmt::format("NameMismatchActn  = {}\n", xActn2Str(m_NameMismatchActn));
  //operation
  Config += fmt::format("CalkPSNR          = {:d}\n", m_CalkPSNR);
  //Config += fmt::format("NumberOfThreads   = {}{}\n", m_NumberOfThreads, m_NumberOfThreads == NOT_VALID ? "  (all)" : "");
  Config += fmt::format("VerboseLevel      = {}\n", m_VerboseLevel);
  Config += "\n";
  //derrived
  Config += fmt::format("Run-time derrived parameters:\n");
  Config += fmt::format("WriteBitstream    = {:d}\n", m_WriteBit  );
  Config += fmt::format("WriteRecon        = {:d}\n", m_WriteRecon);
  Config += fmt::format("PerformDecoding   = {:d}\n", m_Decode    );
  Config += fmt::format("ConvertColorspace = {:d}\n", m_CvtClrSpc );
  Config += fmt::format("PictureMargin     = {:d}\n", m_PicMargin );
  Config += fmt::format("PrintFrame        = {:d}\n", m_PrintFrame);
  Config += fmt::format("GatherTime        = {:d}\n", m_GatherTime);
  Config += fmt::format("PrintDebug        = {:d}\n", m_PrintDebug);

  return Config;
}
eAppRes xAppJPEG::validateInputFiles()
{
  bool AnyError = false;

  if(m_NameMismatchActn == eActn::WARN || m_NameMismatchActn == eActn::STOP)
  {
    const auto [ValidI, MessageI] = xFileNameScn::validateFileParams(m_InputFile, m_PictureSize, m_BitDepth, m_ChromaFormat);
    if(!ValidI) { m_ErrorLog += MessageI; AnyError = true; }
  }

  if(AnyError) { return m_NameMismatchActn == eActn::STOP ? eAppRes::Error : eAppRes::Warning; }
  return eAppRes::Good;
}
std::string xAppJPEG::formatWarnings()
{
  std::string Warnings = "";
  return Warnings;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

eAppRes xAppJPEG::setupSeqAndBuffs()
{
  //check if file exists
  if(m_FileFormat == eFileFmt::RAW)
  {
    if(!xFile::exists(m_InputFile)) { xCfgINI::printError(fmt::format("ERROR --> InputFile does not exist ({})", m_InputFile)); return eAppRes::Error; }
  }
  else
  {
    const std::string InputFile0 = fmt::format(m_InputFile, 0);
    const std::string InputFile1 = fmt::format(m_InputFile, 1);
    if(!xFile::exists(InputFile0) && !xFile::exists(InputFile1)) { xCfgINI::printError(fmt::format("ERROR --> InputFile does not exist ({}) [Checked {} {}]", m_InputFile, InputFile0, InputFile1)); return eAppRes::Error; }
  }

  //file size
  if(m_FileFormat == eFileFmt::RAW)
  {
    int64 SizeOfInputFile = xFile::size(m_InputFile);
    if(m_VerboseLevel >= 1) { fmt::print("SizeOfInputFile = {}\n", SizeOfInputFile); }
  }

  //input sequence 
  const eCrF TmpChromaFormat = m_PictureType == eImgTp::RGB ? eCrF::CF444 : m_ChromaFormat;
  switch(m_FileFormat)
  {
  case eFileFmt::RAW: m_SeqOrg = new xSeqRAW(m_PictureSize, m_BitDepth, TmpChromaFormat); break;
  case eFileFmt::PNG: m_SeqOrg = new xSeqPNG(m_PictureSize, uint16_max                 ); break;
  case eFileFmt::BMP: m_SeqOrg = new xSeqBMP(m_PictureSize, uint16_max                 ); break;
  default: xCfgINI::printError(fmt::format("ERROR --> unsupported FileFormat ({})", xFileFmt2Str(m_FileFormat))); return eAppRes::Error;
  }
  {
    xSeqBase::tResult Result = m_SeqOrg->openFile(m_InputFile, xSeq::eMode::Read);
    if(!Result) { xCfgINI::printError(fmt::format("ERROR --> InputFile opening failure ({}) {}", m_InputFile, Result.format())); return eAppRes::Error; }
  }

  //num of frames per input file
  int32 NumOfFrames = 0;
  {
    NumOfFrames = m_SeqOrg->getNumOfFrames();
    if(m_VerboseLevel >= 1) { fmt::print("DetectedFrames = {}\n", NumOfFrames); }
    if(m_StartFrame >= NumOfFrames) { xCfgINI::printError(fmt::format("ERROR --> StartFrame >= DetectedFrames for ({})", m_InputFile)); return eAppRes::Error; }
  }

  //num of frames to process
  int32 SeqNumFrames = NumOfFrames;
  int32 SeqRemFrames = NumOfFrames - m_StartFrame;
  m_NumFrames        = xMin(m_NumberOfFrames > 0 ? m_NumberOfFrames : SeqNumFrames, SeqRemFrames);
  int32 FirstFrame   = xMin(m_StartFrame, NumOfFrames - 1);
  if(m_VerboseLevel >= 1) { fmt::print("FramesToProcess  = {}\n", m_NumFrames); }
  fmt::print("\n");

  //seeek sequences 
  if(FirstFrame != 0)
  {
    xSeqBase::tResult Result = m_SeqOrg->seekFrame(FirstFrame);
    if(!Result) { xCfgINI::printError(fmt::format("ERROR --> InputFile seeking failure ({}) {}", m_InputFile, Result.format())); return eAppRes::Error; }
  }

  //output sequence
  if(m_WriteRecon) 
  {
    switch(m_FileFormat)
    {
    case eFileFmt::RAW: m_SeqRec = new xSeqRAW(m_PictureSize, m_BitDepth, TmpChromaFormat); break;
    case eFileFmt::PNG: m_SeqRec = new xSeqPNG(m_PictureSize, uint16_max                 ); break;
    case eFileFmt::BMP: m_SeqRec = new xSeqBMP(m_PictureSize, uint16_max                 ); break;
    default: xCfgINI::printError(fmt::format("ERROR --> unsupported FileFormat ({})", xFileFmt2Str(m_FileFormat))); return eAppRes::Error;
    }

    xSeqBase::tResult Result = m_SeqRec->openFile(m_ReconFile, xSeq::eMode::Write);
    if(!Result) { xCfgINI::printError(fmt::format("ERROR --> ReconFile opening failure ({}) {}", m_ReconFile, Result.format())); return eAppRes::Error; }
  }
  
  //buffers
  m_PicOrg4XX = new xPicYUV(m_PictureSize, m_BitDepth, m_ChromaFormat, m_PicMargin);
  if(m_Decode) { m_PicRec4XX = new xPicYUV(m_PictureSize, m_BitDepth, m_ChromaFormat, m_PicMargin); }
  if(m_PictureType == eImgTp::RGB)
  {
    m_PicOrgRGB = new xPicP(m_PictureSize, m_BitDepth, 0);
    if(m_Decode) { m_PicRecRGB = new xPicP(m_PictureSize, m_BitDepth, 0); }
    if(m_ChromaFormat != eCrF::CF444)
    {
      m_PicOrg444 = new xPicYUV(m_PictureSize, m_BitDepth, eCrF::CF444, m_PicMargin);
      if(m_Decode) { m_PicRec444 = new xPicYUV(m_PictureSize, m_BitDepth, eCrF::CF444, m_PicMargin); }
    }
  }

  //byte buffer & file
  m_OutBuffer.create(m_PictureSize.getMul() * 4);
  if(m_WriteBit)
  {
    bool OpenSucces = m_OutFile.openFile(m_OutputFile, xStream::eMode::Write);
    if(!OpenSucces) { xCfgINI::printError(fmt::format("ERROR --> OutputFile opening failure ({})", m_OutputFile)); return eAppRes::Error; }
  }

  return eAppRes::Good;
}
eAppRes xAppJPEG::ceaseSeqAndBuffs()
{
  if(m_WriteBit) { m_OutFile.flush(); m_OutFile.closeFile(); }
  //TODO

  return eAppRes::Good;
}
void xAppJPEG::createProcessors()
{
  switch(m_Implementation)
  {
  case eImpl::Simple:
    m_EncoderSimple.setVerboseLevel(m_VerboseLevel);
    m_EncoderSimple.create();
    m_EncoderSimple.init(m_PictureSize, m_ChromaFormat, m_Quality, m_RestartInterval, true, true, true);
    m_EncoderSimple.setGatherTimeStats(m_PrintDebug);
    if(m_Decode)
    {
      m_DecoderSimple.setVerboseLevel(m_VerboseLevel);
      m_DecoderSimple.create();
      m_DecoderSimple.setGatherTimeStats(m_PrintDebug);
    }
    break;
  case eImpl::Advanded:
    m_EncoderRDOQ.setVerboseLevel(m_VerboseLevel);
    m_EncoderRDOQ.create(m_PictureSize, m_ChromaFormat);
    m_EncoderRDOQ.initBaseMarkers();
    m_EncoderRDOQ.initQuant(m_Quality, m_QuantTabLayout);
    m_EncoderRDOQ.initEntropy(m_RestartInterval);
    m_EncoderRDOQ.setMarkerEmit(true, true, true);
    m_EncoderRDOQ.setRDOQ(m_OptimizeLuma, m_OptimizeChroma, m_ProcessZeroCoeffs, m_NumBlockOptPasses);
    m_EncoderRDOQ.setGatherTimeStats(m_PrintDebug);
    if(m_Decode)
    {
      m_DecoderSimple.setVerboseLevel(m_VerboseLevel);
      m_DecoderSimple.create();
      m_DecoderSimple.setGatherTimeStats(m_PrintDebug);
    }
    break;
  default: assert(0); break;
  }

  m_FramePSNR_YUV.resize(m_NumFrames, xMakeVec4<flt64>(std::numeric_limits<flt64>::quiet_NaN()));
  if(m_CvtClrSpc) { m_FramePSNR_RGB.resize(m_NumFrames, xMakeVec4<flt64>(std::numeric_limits<flt64>::quiet_NaN())); }
  m_FrameBits    .resize(m_NumFrames, 0);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

eAppRes xAppJPEG::processAllFrames()
{
  m_ProcBegTime = tClock::now(); m_ProcBegTicks = xTSC();

  for(int32 f = 0; f < m_NumFrames; f++)
  {
    if(m_PrintDebug) { fmt::print("Frame {:08d} ", f); }

    uint64 T0 = m_GatherTime ? xTSC() : 0;

    //reading
    xSeqBase::tResult ReadResult = !m_CvtClrSpc ? m_SeqOrg->readFrame(m_PicOrg4XX) : m_SeqOrg->readFrame(m_PicOrgRGB);
    if(!ReadResult) { xCfgINI::printError(fmt::format("ERROR --> InputFile read error ({}) {}", m_InputFile, ReadResult.format())); return eAppRes::Error; }
    if(m_ReorderRGB) { reorderRGB(); }

    uint64 T1 = m_GatherTime ? xTSC() : 0;

    //validation
    if(m_InvalidPelActn != eActn::SKIP)
    {
      eAppRes ValidationRes = validateFrames();
      if(ValidationRes != eAppRes::Good) { return eAppRes::Error; }
    }

    uint64 T2 = m_GatherTime ? xTSC() : 0;

    if(m_CvtClrSpc) { cvtRGBtoYCbCr(); }

    uint64 T3 = m_GatherTime ? xTSC() : 0;

    //encoding
    m_OutBuffer.reset();
    switch(m_Implementation)
    {
      case eImpl::Simple  : m_EncoderSimple.encode(m_PicOrg4XX, &m_OutBuffer); break;
      case eImpl::Advanded: m_EncoderRDOQ  .encode(m_PicOrg4XX, &m_OutBuffer); break;
    }    

    uint64 T4 = m_GatherTime ? xTSC() : 0;

    //writting output
    m_FrameBits[f] = m_OutBuffer.getDataSize() << 3;
    if(m_WriteBit) { m_OutBuffer.write(&m_OutFile); }
    m_OutFile.flush();

    uint64 T5 = m_GatherTime ? xTSC() : 0;

    //jpeg decompression
    if(m_Decode)
    {
      switch(m_Implementation)
      {
      case eImpl::Simple  : m_DecoderSimple.init(&m_OutBuffer); m_DecoderSimple.decode(&m_OutBuffer, m_PicRec4XX); break;
      case eImpl::Advanded: m_DecoderSimple.init(&m_OutBuffer); m_DecoderSimple.decode(&m_OutBuffer, m_PicRec4XX); break;
      }      
    }

    uint64 T6 = m_GatherTime ? xTSC() : 0;

    if(m_CalkPSNR) { m_FramePSNR_YUV[f] = calcPicPSNR(m_PicRec4XX, m_PicOrg4XX, true); }

    uint64 T7 = m_GatherTime ? xTSC() : 0;

    if(m_CvtClrSpc) { cvtYCbCrToRGB(); }

    uint64 T8 = m_GatherTime ? xTSC() : 0;

    if(m_CalkPSNR && m_CvtClrSpc) { m_FramePSNR_RGB[f] = calcPicPSNR(m_PicRecRGB, m_PicOrgRGB, true); }

    uint64 T9 = m_GatherTime ? xTSC() : 0;

    //write recon
    if(m_WriteRecon)
    {
      if(m_ReorderRGB) { reorderRGB(); }
      xSeqBase::tResult WriteResult = !m_CvtClrSpc ? m_SeqRec->writeFrame(m_PicRec4XX) : m_SeqRec->writeFrame(m_PicRecRGB);
      if(!WriteResult) { xCfgINI::printError(fmt::format("ERROR --> InputFile write error ({}) {}", m_ReconFile, WriteResult.format())); return eAppRes::Error; }
    }

    uint64 T10 = m_GatherTime ? xTSC() : 0;

    if(m_GatherTime)
    {
      m_Ticks_LoadOrg += T1  - T0;
      m_TicksValidate += T2  - T1;
      m_Ticks_RGB2YUV += T3  - T2;
      m_Ticks__Encode += T4  - T3;
      m_TicksWriteBit += T5  - T4;
      m_Ticks__Decode += T6  - T5;
      m_Ticks_YUV2RGB += T8  - T7;
      m_TicksCalcPSNR += (T7  - T6) + (T9 - T8);
      m_TicksWriteRec += T10 - T9;      
    }

    if(m_PrintDebug)
    { 
      if(m_CalkPSNR)
      {
        fmt::print("PSNR[dB]: Y={:2.2f} Cb={:2.2f} Cr={:2.2f}", m_FramePSNR_YUV[f][0], m_FramePSNR_YUV[f][1], m_FramePSNR_YUV[f][2]);
        if(m_CvtClrSpc) { fmt::print(" R={:2.2f} G={:2.2f} B={:2.2f} ", m_FramePSNR_RGB[f][0], m_FramePSNR_RGB[f][1], m_FramePSNR_RGB[f][2]); }
      }
      fmt::print("Size={:d}B ", m_FrameBits[f]>>3);
      fmt::print("\n");
    }
  }

  m_ProcEndTime = tClock::now(); m_ProcEndTicks = xTSC();

  return eAppRes::Good;
}
void xAppJPEG::reorderRGB()
{
  if(m_PictureType == eImgTp::BGR)
  {
    m_PicOrgRGB->swapComponents(eCmp::C0, eCmp::C2); //BGR --> RGB 
  }
  if(m_PictureType == eImgTp::GBR)
  {
    m_PicOrgRGB->swapComponents(eCmp::C0, eCmp::C1); //GBR --> BGR
    m_PicOrgRGB->swapComponents(eCmp::C0, eCmp::C2); //BGR --> RGB
  }
}
eAppRes xAppJPEG::validateFrames()
{
  bool CheckOK = true;
  if(!m_CvtClrSpc)
  {
    CheckOK = m_PicOrg4XX->check(m_InputFile);
    if(m_InvalidPelActn == eActn::CNCL && !CheckOK) { m_PicOrg4XX->conceal(); }
  }
  else
  {
    CheckOK = m_PicOrgRGB->check(m_InputFile);
    if(m_InvalidPelActn == eActn::CNCL && !CheckOK) { m_PicOrgRGB->conceal(); }
  }

  if(m_InvalidPelActn==eActn::STOP && !CheckOK) { xCfgINI::printError(fmt::format("ERROR --> InputFile contains invalid values ({})", m_InputFile)); return eAppRes::Error; }

  return eAppRes::Good;
}
void xAppJPEG::cvtRGBtoYCbCr()
{
  if(m_ChromaFormat == eCrF::CF444)
  {
    xColorSpace::ConvertRGB2YCbCr(
      m_PicOrg4XX->getAddr(eCmp::LM), m_PicOrg4XX->getAddr(eCmp::CB), m_PicOrg4XX->getAddr(eCmp::CR),
      m_PicOrgRGB->getAddr(eCmp::R), m_PicOrgRGB->getAddr(eCmp::G), m_PicOrgRGB->getAddr(eCmp::B),
      m_PicOrg4XX->getStride(eCmp::LM), m_PicOrgRGB->getStride(), m_PicOrg4XX->getWidth(eCmp::LM), m_PicOrg4XX->getHeight(eCmp::LM),
      m_PicOrg4XX->getBitDepth(), eClrSpcLC::BT601);
  }
  else
  {
    xColorSpace::ConvertRGB2YCbCr(
      m_PicOrg444->getAddr(eCmp::LM), m_PicOrg444->getAddr(eCmp::CB), m_PicOrg444->getAddr(eCmp::CR),
      m_PicOrgRGB->getAddr(eCmp::R), m_PicOrgRGB->getAddr(eCmp::G), m_PicOrgRGB->getAddr(eCmp::B),
      m_PicOrg444->getStride(eCmp::LM), m_PicOrgRGB->getStride(), m_PicOrg444->getWidth(eCmp::LM), m_PicOrg444->getHeight(eCmp::LM),
      m_PicOrg444->getBitDepth(), eClrSpcLC::JPEG);

    m_PicOrg4XX->swapBuffer(m_PicOrg444, eCmp::LM);

    if(m_ChromaFormat == eCrF::CF422)
    {
      xPixelOps::DownsampleH(m_PicOrg4XX->getAddr(eCmp::CB), m_PicOrg444->getAddr(eCmp::CB), m_PicOrg4XX->getStride(eCmp::CB), m_PicOrg444->getStride(eCmp::CB), m_PicOrg4XX->getWidth(eCmp::CB), m_PicOrg4XX->getHeight(eCmp::CB));
      xPixelOps::DownsampleH(m_PicOrg4XX->getAddr(eCmp::CR), m_PicOrg444->getAddr(eCmp::CR), m_PicOrg4XX->getStride(eCmp::CR), m_PicOrg444->getStride(eCmp::CR), m_PicOrg4XX->getWidth(eCmp::CR), m_PicOrg4XX->getHeight(eCmp::CR));
    }
    else //420
    {
      xPixelOps::DownsampleHV(m_PicOrg4XX->getAddr(eCmp::CB), m_PicOrg444->getAddr(eCmp::CB), m_PicOrg4XX->getStride(eCmp::CB), m_PicOrg444->getStride(eCmp::CB), m_PicOrg4XX->getWidth(eCmp::CB), m_PicOrg4XX->getHeight(eCmp::CB));
      xPixelOps::DownsampleHV(m_PicOrg4XX->getAddr(eCmp::CR), m_PicOrg444->getAddr(eCmp::CR), m_PicOrg4XX->getStride(eCmp::CR), m_PicOrg444->getStride(eCmp::CR), m_PicOrg4XX->getWidth(eCmp::CR), m_PicOrg4XX->getHeight(eCmp::CR));
    }
  }
}
void xAppJPEG::cvtYCbCrToRGB()
{
  if(m_ChromaFormat == eCrF::CF444)
  {
    xColorSpace::ConvertYCbCr2RGB(
      m_PicRecRGB->getAddr(eCmp::R ), m_PicRecRGB->getAddr(eCmp::G ), m_PicRecRGB->getAddr(eCmp::B ), 
      m_PicRec4XX->getAddr(eCmp::LM), m_PicRec4XX->getAddr(eCmp::CB), m_PicRec4XX->getAddr(eCmp::CR),
      m_PicRecRGB->getStride(), m_PicRec4XX->getStride(eCmp::LM), m_PicRecRGB->getWidth(), m_PicRecRGB->getHeight(),
      m_PicRecRGB->getBitDepth(), eClrSpcLC::JPEG);
  }
  else
  {
    if(m_ChromaFormat == eCrF::CF422)
    {
      xPixelOps::UpsampleH(m_PicRec444->getAddr(eCmp::CB), m_PicRec4XX->getAddr(eCmp::CB), m_PicRec444->getStride(eCmp::CB), m_PicRec4XX->getStride(eCmp::CB), m_PicRec444->getWidth(eCmp::CB), m_PicRec444->getHeight(eCmp::CB));
      xPixelOps::UpsampleH(m_PicRec444->getAddr(eCmp::CR), m_PicRec4XX->getAddr(eCmp::CR), m_PicRec444->getStride(eCmp::CR), m_PicRec4XX->getStride(eCmp::CR), m_PicRec444->getWidth(eCmp::CR), m_PicRec444->getHeight(eCmp::CR));
    }
    else //420
    {
      xPixelOps::UpsampleHV(m_PicRec444->getAddr(eCmp::CB), m_PicRec4XX->getAddr(eCmp::CB), m_PicRec444->getStride(eCmp::CB), m_PicRec4XX->getStride(eCmp::CB), m_PicRec444->getWidth(eCmp::CB), m_PicRec444->getHeight(eCmp::CB));
      xPixelOps::UpsampleHV(m_PicRec444->getAddr(eCmp::CR), m_PicRec4XX->getAddr(eCmp::CR), m_PicRec444->getStride(eCmp::CR), m_PicRec4XX->getStride(eCmp::CR), m_PicRec444->getWidth(eCmp::CR), m_PicRec444->getHeight(eCmp::CR));
    }

    m_PicRec444->swapBuffer(m_PicRec4XX, eCmp::LM);

    xColorSpace::ConvertYCbCr2RGB(m_PicRecRGB->getAddr(eCmp::R), m_PicRecRGB->getAddr(eCmp::G), m_PicRecRGB->getAddr(eCmp::B),
      m_PicRec444->getAddr(eCmp::LM), m_PicRec444->getAddr(eCmp::CB), m_PicRec444->getAddr(eCmp::CR),
      m_PicRecRGB->getStride(), m_PicRec444->getStride(eCmp::LM), m_PicRecRGB->getWidth(), m_PicRecRGB->getHeight(),
      m_PicRecRGB->getBitDepth(), eClrSpcLC::BT601);
  }
}
flt64V4 xAppJPEG::calcPicPSNR(const xPicP* Tst, const xPicP* Ref, bool AvoidInfPSNR)
{
  assert(Ref != nullptr && Tst != nullptr);
  assert(Ref->isCompatible(Tst));

  flt64V4 PSNR = xMakeVec4(flt64_max);
  for(int32 CmpIdx = 0; CmpIdx < Tst->getNumCmps(); CmpIdx++)
  {
    PSNR[CmpIdx] = calcCmpPSNR(Tst, Ref, (eCmp)CmpIdx, AvoidInfPSNR);
  }
  return PSNR;
}
flt64 xAppJPEG::calcCmpPSNR(const xPicP* Tst, const xPicP* Ref, eCmp CmpId, bool AvoidInfPSNR)
{
  const int32   Width     = Ref->getWidth ();
  const int32   Height    = Ref->getHeight();
  const uint16* TstPtr    = Tst->getAddr  (CmpId);
  const uint16* RefPtr    = Ref->getAddr  (CmpId);
  const int32   TstStride = Tst->getStride();
  const int32   RefStride = Ref->getStride();

  uint64 SSD = xDistortion::CalcSSD(TstPtr, RefPtr, TstStride, RefStride, Width, Height);
  if(AvoidInfPSNR && SSD == 0) { SSD = 1; }

  uint64 NumPoints = Width * Height;
  uint64 MaxValue  = xBitDepth2MaxValue(Tst->getBitDepth());
  uint64 MAX       = (NumPoints) * xPow2(MaxValue);
  flt64  PSNR      = SSD > 0 ? 10.0 * log10((flt64)MAX / SSD) : flt64_max;

  return PSNR;
}
flt64V4 xAppJPEG::calcPicPSNR(const xPicYUV* Tst, const xPicYUV* Ref, bool AvoidInfPSNR)
{
  assert(Ref != nullptr && Tst != nullptr);
  assert(Ref->isCompatible(Tst));

  flt64V4 PSNR = xMakeVec4(flt64_max);
  for(int32 CmpIdx = 0; CmpIdx < Tst->getNumCmps(); CmpIdx++)
  {
    PSNR[CmpIdx] = calcCmpPSNR(Tst, Ref, (eCmp)CmpIdx, AvoidInfPSNR);
  }
  return PSNR;
}
flt64 xAppJPEG::calcCmpPSNR(const xPicYUV* Tst, const xPicYUV* Ref, eCmp CmpId, bool AvoidInfPSNR)
{
  const int32   Width     = Ref->getWidth (CmpId);
  const int32   Height    = Ref->getHeight(CmpId);
  const uint16* TstPtr    = Tst->getAddr  (CmpId);
  const uint16* RefPtr    = Ref->getAddr  (CmpId);
  const int32   TstStride = Tst->getStride(CmpId);
  const int32   RefStride = Ref->getStride(CmpId);

  uint64 SSD = xDistortion::CalcSSD(TstPtr, RefPtr, TstStride, RefStride, Width, Height);
  if(AvoidInfPSNR && SSD == 0) { SSD = 1; }

  uint64 NumPoints = Width * Height;
  uint64 MaxValue  = xBitDepth2MaxValue(Tst->getBitDepth());
  uint64 MAX       = (NumPoints) * xPow2(MaxValue);
  flt64  PSNR      = SSD > 0 ? 10.0 * log10((flt64)MAX / SSD) : flt64_max;

  return PSNR;
}
std::string xAppJPEG::calibrateTimeStamp()
{
  tDuration TotalProcTime  = m_ProcEndTime  - m_ProcBegTime ;
  uint64    TotalProcTicks = m_ProcEndTicks - m_ProcBegTicks;

  flt64 TicksPerMicroSec = (flt64)TotalProcTicks / std::chrono::duration_cast<tDurationUS>(TotalProcTime).count();
  flt64 TicksPerMiliSec  = (flt64)TotalProcTicks / std::chrono::duration_cast<tDurationMS>(TotalProcTime).count();
  flt64 TicksPerSec      = (flt64)TotalProcTicks / std::chrono::duration_cast<tDurationS >(TotalProcTime).count();

  std::string Result = fmt::format("CalibratedTicksPerSec = {:.0f} ({:.3f}MHz)\n", TicksPerSec, TicksPerMicroSec);

  m_InvDurationDenominator = (flt64)1.0 / ((flt64)m_NumFrames * TicksPerMiliSec);

  return Result;
}
void xAppJPEG::combineFrameStats()
{
  m_AvgPSNR_YUV       = xKBNS::Accumulate(m_FramePSNR_YUV) / m_NumFrames;
  if(m_CvtClrSpc) { m_AvgPSNR_RGB = xKBNS::Accumulate(m_FramePSNR_RGB) / m_NumFrames; }
  uint64 TotalBits    = std::accumulate(m_FrameBits.begin(), m_FrameBits.end(), (uint64)0);
  int32  OneFrameSize = m_SeqOrg->getOneFrameSize();
  m_AvgFrameBytes     = (flt64)(TotalBits) / (flt64)(m_NumFrames * 8);
  m_Bitrate           = (flt64)(TotalBits*m_FrameRate)/((flt64)(m_NumFrames));
  m_BitsPerPixel      = (flt64)(TotalBits)/(flt64)((uint64)m_NumFrames*m_PictureSize.getMul());
  m_ComprRatio        = m_AvgFrameBytes / (flt64)(OneFrameSize);
}

std::string xAppJPEG::formatResultsStdOut()
{
  std::string Result; Result.reserve(xMemory::c_MemSizePageBase);

  Result += "SUMMARY:\n";
  Result += fmt::format("TotalFrames         = {}\n"            , m_NumFrames     );
  Result += fmt::format("Bitrate             = {:.3f} kib/s\n"  , m_Bitrate/1024  );
  Result += fmt::format("PSNR-Y              = {:10.6f} dB\n"   , m_AvgPSNR_YUV[0]);
  Result += fmt::format("PSNR-Cb             = {:10.6f} dB\n"   , m_AvgPSNR_YUV[1]);
  Result += fmt::format("PSNR-Cr             = {:10.6f} dB\n"   , m_AvgPSNR_YUV[2]);
  if(m_CvtClrSpc)
  {
    Result += fmt::format("PSNR-R              = {:10.6f} dB\n", m_AvgPSNR_RGB[0]);
    Result += fmt::format("PSNR-G              = {:10.6f} dB\n", m_AvgPSNR_RGB[1]);
    Result += fmt::format("PSNR-B              = {:10.6f} dB\n", m_AvgPSNR_RGB[2]);
  }
  Result += fmt::format("AverageFrameSize    = {:.4f} Bytes\n"     , m_AvgFrameBytes);
  Result += fmt::format("AverageBitsPerPixel = {:.4f} Bits/Pixel\n", m_BitsPerPixel );
  Result += fmt::format("CompressionRatio    = {:.4f}\n"           , m_ComprRatio   );

  if(m_GatherTime)
  {
    tDurationUS AvgDuration_LoadOrg = tDurationMS((flt64)m_Ticks_LoadOrg * m_InvDurationDenominator);
    tDurationUS AvgDurationValidate = tDurationMS((flt64)m_TicksValidate * m_InvDurationDenominator);
    tDurationUS AvgDuration_RGB2YUV = tDurationMS((flt64)m_Ticks_RGB2YUV * m_InvDurationDenominator);
    tDurationUS AvgDuration__Encode = tDurationMS((flt64)m_Ticks__Encode * m_InvDurationDenominator);
    tDurationUS AvgDurationWriteBit = tDurationMS((flt64)m_TicksWriteBit * m_InvDurationDenominator);
    tDurationUS AvgDuration__Decode = tDurationMS((flt64)m_Ticks__Decode * m_InvDurationDenominator);
    tDurationUS AvgDuration_YUV2RGB = tDurationMS((flt64)m_Ticks_YUV2RGB * m_InvDurationDenominator);
    tDurationUS AvgDurationCalcPSNR = tDurationMS((flt64)m_TicksCalcPSNR * m_InvDurationDenominator);
    tDurationUS AvgDurationWriteRec = tDurationMS((flt64)m_TicksWriteRec * m_InvDurationDenominator);    

    Result += "\nTIME:\n";
                       Result += fmt::format("AvgTime       LoadOrg {:9.2f} us\n", AvgDuration_LoadOrg.count());
    if(m_Validate  ) { Result += fmt::format("AvgTime      Validate {:9.2f} us\n", AvgDurationValidate.count()); }
    if(m_CvtClrSpc ) { Result += fmt::format("AvgTime       RGB2YUV {:9.2f} us\n", AvgDuration_RGB2YUV.count()); }
                       Result += fmt::format("AvgTime        Encode {:9.2f} us\n", AvgDuration__Encode.count());
    if(m_WriteBit  ) { Result += fmt::format("AvgTime      WriteBit {:9.2f} us\n", AvgDurationWriteBit.count()); }
    if(m_Decode    ) { Result += fmt::format("AvgTime        Decode {:9.2f} us\n", AvgDuration__Decode.count()); }
    if(m_CvtClrSpc ) { Result += fmt::format("AvgTime       YUV2RGB {:9.2f} us\n", AvgDuration_YUV2RGB.count()); }
    if(m_CalkPSNR  ) { Result += fmt::format("AvgTime      CalcPSNR {:9.2f} us\n", AvgDurationCalcPSNR.count()); }
    if(m_WriteRecon) { Result += fmt::format("AvgTime      WriteRec {:9.2f} us\n", AvgDurationWriteRec.count()); }
  }

//  if(m_PrintDebug)
//  {
//    Result += fmt::format("\nENCODER TIME STATS:\n");
//    switch(m_Implementation)
//    {
//    case 0: Result += m_EncoderSimple.formatAndResetStats("  ") + "\n"; break;
//    case 1: Result += m_EncoderRDOQ  .formatAndResetStats("  ") + "\n"; break;
//#if X_PMBB_HAS_JPEG_TURBO
//    case 2: Result += m_EncoderTurbo .formatAndResetStats("  ") + "\n"; break;
//#endif //X_PMBB_HAS_JPEG_TURBO
//    }
//    if(m_Decode)
//    {
//      Result += fmt::format("DECODER TIME STATS:\n");
//      switch(m_Implementation)
//      {
//      case 0: Result += m_DecoderSimple.formatAndResetStats("  ") + "\n"; break;
//      case 1: Result += m_DecoderSimple.formatAndResetStats("  ") + "\n"; break;
//#if X_PMBB_HAS_JPEG_TURBO
//      case 2: Result += m_DecoderTurbo .formatAndResetStats("  ") + "\n"; break;
//#endif //X_PMBB_HAS_JPEG_TURBO
//      }
//    }
//  }
  return Result;
}

//===============================================================================================================================================================================================================

} //end of namespace PMBB::JPEG