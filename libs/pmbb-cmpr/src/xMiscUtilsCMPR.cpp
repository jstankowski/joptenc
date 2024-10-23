/*
    SPDX-FileCopyrightText: 2019-2023 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xMiscUtilsCMPR.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================

std::string xMiscUtilsCMPR::formatCompileTimeSetup()
{
  std::string Str;
  Str += fmt::format("CMPR_TRACE_LIKE_HM     = {:d}\n", PMBB_CMPR_TRACE_LIKE_HM);
  Str += fmt::format("CMPR_TRACE_FLUSH       = {:d}\n", PMBB_CMPR_TRACE_FLUSH  );
  return Str;
}

//===============================================================================================================================================================================================================

} //end of namespace PMBB
