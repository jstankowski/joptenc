/*
    SPDX-FileCopyrightText: 2019-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

//===============================================================================================================================================================================================================

#include "xAppJOptEnc.h"
#include "xFmtScn.h"
#include <filesystem>

using namespace PMBB_NAMESPACE;
using namespace PMBB_NAMESPACE::JPEG;

//===============================================================================================================================================================================================================
// Main
//===============================================================================================================================================================================================================
#ifndef APP_MAIN
#define APP_MAIN main
#endif

int32 APP_MAIN(int argc, char* argv[], char* /*envp*/[])
{
  fmt::printf("%s\n", xAppJPEG::c_BannerString);
  tTimePoint AppBeg = tClock::now();
  xAppJPEG AppJPEG;

  //===================================================================================================================
  //configuring
  //===================================================================================================================
  
  //parsing configuration
  AppJPEG.registerCmdParams();
  bool CfgLoadResult = AppJPEG.loadConfiguration(argc, const_cast<const char**>(argv));
  if(!CfgLoadResult) { xCfgINI::printError(AppJPEG.getErrorLog() + "\n\n", xAppJPEG::c_HelpString); return EXIT_FAILURE; }
  bool CfgReadResult = AppJPEG.readConfiguration();
  if(!CfgReadResult) { xCfgINI::printError(AppJPEG.getErrorLog() + "\n\n", xAppJPEG::c_HelpString); return EXIT_FAILURE; }
  const int32 VerboseLevel = AppJPEG.getVerboseLevel();

  if(VerboseLevel >= 2)
  { 
    fmt::print("WorkingDir = " + std::filesystem::current_path().string() + "\n\n");    
    fmt::print("Commandline args:\n"); xCfgINI::printCommandlineArgs(argc, const_cast<const char**>(argv));
  }

  //print compile time setup
  if (VerboseLevel >= 1)
  {
    fmt::print(xMiscUtilsCORE::formatCompileTimeSetup());
    fmt::print("\n");
  }

  //print config
  if(VerboseLevel >= 1) { fmt::print(AppJPEG.formatConfiguration()); fmt::print("\n"); }

  //validate file names against input parameters
  eAppRes ValidFilesRes = AppJPEG.validateInputFiles();
  if(ValidFilesRes == eAppRes::Warning) { xCfgINI::printError(std::string("PARAMETERS WARNING: Invalid parameters\n") + AppJPEG.getErrorLog()); }
  if(ValidFilesRes == eAppRes::Error  ) { xCfgINI::printError(std::string("PARAMETERS WARNING: Invalid parameters\n") + AppJPEG.getErrorLog()); return EXIT_FAILURE; }

  //print configuration warnings
  std::string ConfigWarnings = AppJPEG.formatWarnings();
  if(!ConfigWarnings.empty()) { fmt::print(ConfigWarnings); }

  //spacer
  fmt::print("\n\n\n");

  //===================================================================================================================
  // preparation
  //===================================================================================================================
  if(VerboseLevel >= 2) { fmt::print("Initializing:\n"); }

  eAppRes SeqRes = AppJPEG.setupSeqAndBuffs();
  if(SeqRes == eAppRes::Error) { return EXIT_FAILURE; }

  AppJPEG.createProcessors();

  //===================================================================================================================
  //running
  //===================================================================================================================
  tTimePoint PrcBeg = tClock::now();
  eAppRes ClcRes = AppJPEG.processAllFrames();
  if(ClcRes == eAppRes::Error) { return EXIT_FAILURE; }
  tTimePoint PrcEnd = tClock::now();

  //===================================================================================================================
  //finalizing
  //===================================================================================================================
  if(VerboseLevel >= 1) { fmt::print("\n"); fmt::print(AppJPEG.calibrateTimeStamp()); }
  fmt::print("\n\n");
  AppJPEG.combineFrameStats  ();
  AppJPEG.ceaseSeqAndBuffs   ();

  //printout results
  fmt::print(AppJPEG.formatResultsStdOut());
  fmt::print("\n");
  tTimePoint AppEnd = tClock::now();
  fmt::print("TotalProcessingTime  = {:.3f} s\n", std::chrono::duration_cast<tDurationS>(PrcEnd - PrcBeg).count());
  fmt::print("TotalApplicationTime = {:.3f} s\n", std::chrono::duration_cast<tDurationS>(AppEnd - AppBeg).count());
  fmt::print("END-OF-LOG\n");
  fflush(stdout);

  return EXIT_SUCCESS;
}
