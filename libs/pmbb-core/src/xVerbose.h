/*
    SPDX-FileCopyrightText: 2019-2022 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#pragma once

#include "xCommonDefPMBB.h"

namespace PMBB_NAMESPACE {

//===============================================================================================================================================================================================================

class xVerbose
{
public:
  using tVCB = std::function<void(const std::string&)>;

protected:
  int32 m_VerboseLevel    = 0;
  tVCB  m_VerboseCallback = nullptr;

public:
  void  setVerboseLevel   (int32 VerboseLevel   ) { m_VerboseLevel    = VerboseLevel   ; }
  void  setVerboseCallback(tVCB  VerboseCallback) { m_VerboseCallback = VerboseCallback; }

protected:
  void xMessage(const std::string& Message, int32 Level)
  {
    if(Level >= m_VerboseLevel)
    {
      if(m_VerboseCallback) { m_VerboseCallback(Message); }
      else                  { fmt::printf      (Message); }
    }
  }
};

//===============================================================================================================================================================================================================

} //end of namespace PMBB