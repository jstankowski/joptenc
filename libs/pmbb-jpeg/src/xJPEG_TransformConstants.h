/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_Constants.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

class xTransformConstants
{
public:
  static const     flt32 c_TrM_DCT8x8_FLT[8][8];

  //const bit depth definition - JPEG implemented only for 8 bit/sample
  static constexpr int32 c_BitDepth  = 8;
  static constexpr int32 c_Headroom  = 4; //headroom after forward - to be removed during quantization

  //constants for direct mutiplication method - 8 bit transform coeffs
  static constexpr int32 c_CoeffPrec_8bit    = 6; 
  static constexpr int32 c_FrwShift1st_8bit  = c_CoeffPrec_8bit + c_BitDepth - 12;
  static constexpr int32 c_FrwAdd1st_8bit    = 1<<(c_FrwShift1st_8bit-1); 
  static constexpr int32 c_FrwShift2nd_8bit  = c_CoeffPrec_8bit + 3;
  static constexpr int32 c_FrwAdd2nd_8bit    = 1<<(c_FrwShift2nd_8bit-1); 
  static constexpr int32 c_FwdHeadroom_8bit  = (xJPEG_Constants::c_Log2BlockArea + 2 * c_CoeffPrec_8bit) - (c_FrwShift1st_8bit + c_FrwShift2nd_8bit) - 3;
  static constexpr int32 c_InvShift1st_8bit  = c_CoeffPrec_8bit - 1;
  static constexpr int32 c_InvAdd1st_8bit    = 1<<(c_InvShift1st_8bit-1); 
  static constexpr int32 c_InvShift2nd_8bit  = 12 + c_CoeffPrec_8bit - c_BitDepth;
  static constexpr int32 c_InvAdd2nd_8bit    = 1<<(c_InvShift2nd_8bit-1); 
  static const     int8  c_TrM_DCT8x8_8bit[8][8];
    
  //constants for direct mutiplication method - 16 bit transform coeffs
  static constexpr int32 c_CoeffPrec_16bit   = 14; 
  static constexpr int32 c_FrwShift1st_16bit = c_CoeffPrec_16bit + c_BitDepth - 10;
  static constexpr int32 c_FrwAdd1st_16bit   = 1<<(c_FrwShift1st_16bit-1); 
  static constexpr int32 c_FrwShift2nd_16bit = c_CoeffPrec_16bit + 1;
  static constexpr int32 c_FrwAdd2nd_16bit   = 1<<(c_FrwShift2nd_16bit-1); 
  static constexpr int32 c_FwdHeadroom_16bit = (xJPEG_Constants::c_Log2BlockArea + 2 * c_CoeffPrec_16bit) - (c_FrwShift1st_16bit + c_FrwShift2nd_16bit) - 3;
  static constexpr int32 c_InvShift1st_16bit = c_CoeffPrec_16bit - 1;
  static constexpr int32 c_InvAdd1st_16bit   = 1<<(c_InvShift1st_16bit-1); 
  static constexpr int32 c_InvShift2nd_16bit = 12 + c_CoeffPrec_16bit - c_BitDepth;
  static constexpr int32 c_InvAdd2nd_16bit   = 1<<(c_InvShift2nd_16bit-1); 
  static const     int16 c_TrM_DCT8x8_16bit[8][8];
    
  //constants for fancy butterfly method
  static constexpr int32 c_CoeffPrec_BTF     = 13;
  static constexpr int32 c_FrwShift1st_BTF   = c_CoeffPrec_BTF + c_BitDepth - 12;
  static constexpr int32 c_FrwAdd1st_BTF     = 1<<(c_FrwShift1st_BTF-1); 
  static constexpr int32 c_FrwShift2nd_BTF   = c_CoeffPrec_BTF + 3;
  static constexpr int32 c_FrwAdd2nd_BTF     = 1<<(c_FrwShift2nd_BTF-1); 
  static constexpr int32 c_FwdHeadroom_BTF   = (xJPEG_Constants::c_Log2BlockArea + 2 * c_CoeffPrec_BTF) - (c_FrwShift1st_BTF + c_FrwShift2nd_BTF) - 3;
  static constexpr int32 c_InvShift1st_BTF   = c_CoeffPrec_BTF - 1;
  static constexpr int32 c_InvAdd1st_BTF     = 1<<(c_InvShift1st_BTF-1); 
  static constexpr int32 c_InvShift2nd_BTF   = 12 + c_CoeffPrec_BTF - c_BitDepth;
  static constexpr int32 c_InvAdd2nd_BTF     = 1<<(c_InvShift2nd_BTF-1); 

  static constexpr int16 c_P0o298631336 = (int16)(0.298631336 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P0o390180644 = (int16)(0.390180644 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P0o541196100 = (int16)(0.541196100 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P0o765366865 = (int16)(0.765366865 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P0o899976223 = (int16)(0.899976223 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P1o175875602 = (int16)(1.175875602 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P1o501321110 = (int16)(1.501321110 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P1o847759065 = (int16)(1.847759065 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P1o961570560 = (int16)(1.961570560 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P2o053119869 = (int16)(2.053119869 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P2o562915447 = (int16)(2.562915447 * (1<<c_CoeffPrec_BTF) + 0.5);
  static constexpr int16 c_P3o072711026 = (int16)(3.072711026 * (1<<c_CoeffPrec_BTF) + 0.5);

  //for AVX
  static constexpr int16 c_P1o306562965 = c_P0o541196100 + c_P0o765366865;
  static constexpr int16 c_P0o785694958 = c_P1o175875602 - c_P0o390180644;
  static constexpr int16 c_P0o601344887 = c_P0o899976223 - c_P0o298631336;
  static constexpr int16 c_P0o509795579 = c_P3o072711026 - c_P2o562915447;    

public:
  //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done by subtracting from DC 
  static constexpr int32 c_InvDcCorr = 1024;
  static constexpr int32 c_FwdDcCorr = c_InvDcCorr << c_Headroom;
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG