/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xCommonDefJPEG.h"
#include "xString.h"

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================

eImpl xStrToImpl(const std::string& Impl)
{
  std::string ImplL = xString::toLower(Impl);
  return ImplL == "simple"   ? eImpl::Simple   :
         ImplL == "advanded" ? eImpl::Advanded :
                               eImpl::INVALID  ;
}
std::string xImplToStr(eImpl Impl)
{
  return Impl == eImpl::Simple   ? "Simple"   :
         Impl == eImpl::Advanded ? "Advanded" :
                                   "INVALID"  ;
}
eQTLa xStrToQTLa(const std::string& QTLa)
{
  std::string QTLaL = xString::toLower(QTLa);
  return QTLaL == "default"  ? eQTLa::Default  :
         QTLaL == "flat"     ? eQTLa::Flat     :
         QTLaL == "semiflat" ? eQTLa::SemiFlat :
                               eQTLa::INVALID  ;
}
std::string xQTLaToStr(eQTLa QTLa)
{
  return QTLa == eQTLa::Default  ? "Default"  :
         QTLa == eQTLa::Flat     ? "Flat"     :
         QTLa == eQTLa::SemiFlat ? "SemiFlat" :
                                   "INVALID"  ;
}

//=============================================================================================================================================================================

} //end of namespace PMBB::JPEG