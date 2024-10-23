/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Transform.h"
#include "xJPEG_TransformConstants.h"
#include "xJPEG_Constants.h"
#include "xHelpersSIMD.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

static constexpr int32 PASS1_BITS = 2;
using tTC = xTransformConstants;

//=====================================================================================================================================================================================
// xTransformFLT
//=====================================================================================================================================================================================
void xTransformFLT::FwdTransformDCT_8x8_MFL(int16* restrict Dst, const uint16* Src)
{
  flt32 Tmp[8][8]; //partial transformed

  for(int32 j=0; j < 8; j++) //horizontal transform
  {
    for(int32 u=0; u < 8; u++)
    {
      flt32 Coeff = 0;
      for(int32 x=0; x < 8; x++) { Coeff += (flt32)Src[x] * tTC::c_TrM_DCT8x8_FLT[u][x]; }
      Tmp[u][j] = Coeff;
    }
    Src += 8;
  }

  for(int32 j=0; j < 8; j++) //vertical transform
  {
    for(int32 u=0; u < 8; u++)
    {
      flt32 Coeff = 0;
      for(int32 y=0; y < 8; y++) { Coeff += Tmp[j][y] * tTC::c_TrM_DCT8x8_FLT[u][y]; }
      Dst[u*8] = (int16)(round(Coeff*2));
    }
    Dst++;
  }
}
void xTransformFLT::InvTransformDCT_8x8_MFL(uint16* restrict Dst, const int16* Src)
{
  flt32 Tmp[8][8]; //partial transformed

  for(int32 j=0; j < 8; j++) //vertical transform
  {
    for(int32 u=0; u < 8; u++)
    {
      flt32 Coeff = 0;
      for(int32 y=0; y < 8; y++) { Coeff += (flt32)Src[y*8] * tTC::c_TrM_DCT8x8_FLT[y][u]; }
      Tmp[u][j] = Coeff;
    }
    Src++;
  }

  for(int32 j=0; j < 8; j++) //horizontal transform
  {
    for(int32 u=0; u < 8; u++)
    {
      flt32 Coeff = 0;
      for(int32 x=0; x < 8; x++) { Coeff += Tmp[j][x] * tTC::c_TrM_DCT8x8_FLT[x][u]; }
      Dst[u] = (uint16)xClipU16((int32)round(Coeff/8));
    }
    Dst += 8;
  }
}

//=====================================================================================================================================================================================
// xTransformSTD
//=====================================================================================================================================================================================
void xTransformSTD::FwdTransformDCT_8x8_M16(int16* restrict Dst, const uint16* Src)
{
  int16 Tmp[8][8]; //partial transformed

  for(int32 j=0; j < 8; j++) //horizontal transform
  {
    for(int32 u=0; u < 8; u++)
    {
      int32 Coeff = 0;
      for(int32 x=0; x < 8; x++) { Coeff += (int32)Src[x] * (int32)tTC::c_TrM_DCT8x8_16bit[u][x]; }
      Tmp[u][j] = (int16)((Coeff + tTC::c_FrwAdd1st_16bit) >> tTC::c_FrwShift1st_16bit);
    }
    Src += 8;
  }

  for(int32 j=0; j < 8; j++) //vertical transform
  {
    for(int32 u=0; u < 8; u++)
    {
      int32 Coeff = 0;
      for(int32 y=0; y < 8; y++) { Coeff += (int32)Tmp[j][y] * (int32)tTC::c_TrM_DCT8x8_16bit[u][y]; }
      Dst[u*8] = (int16)((Coeff + tTC::c_FrwAdd2nd_16bit) >> tTC::c_FrwShift2nd_16bit);
    }
    Dst++;
  }
}
void xTransformSTD::InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16* Src)
{
  int16 Tmp[8][8]; //partial transformed

  for(int32 j=0; j < 8; j++) //vertical transform
  {
    for(int32 u=0; u < 8; u++)
    {
      int32 Coeff = 0;
      for(int32 y=0; y < 8; y++) { Coeff += (int32)Src[y*8] * (int32)tTC::c_TrM_DCT8x8_16bit[y][u]; }
      Tmp[u][j] = (int16)((Coeff + tTC::c_InvAdd1st_16bit) >> tTC::c_InvShift1st_16bit);
    }
    Src++;
  }

  for(int32 j=0; j < 8; j++) //horizontal transform
  {
    for(int32 u=0; u < 8; u++)
    {
      int32 Coeff = 0;
      for(int32 x=0; x < 8; x++) { Coeff += (int32)Tmp[j][x] * (int32)tTC::c_TrM_DCT8x8_16bit[x][u]; }
      Dst[u] = (uint16)xClipU16((Coeff + tTC::c_InvAdd2nd_16bit) >> tTC::c_InvShift2nd_16bit);
    }
    Dst += 8;
  }
}
void xTransformSTD::FwdTransformDCT_8x8_BTF(int16* restrict Dst, const uint16* Src)
{  
  const int32 Shift1st  = tTC::c_CoeffPrec_BTF - PASS1_BITS;
  const int32 Add1st    = 1<<(Shift1st-1); 
  const int32 Shift2nd  = tTC::c_CoeffPrec_BTF + PASS1_BITS - 1;
  const int32 Add2nd    = 1<<(Shift2nd-1); 
  const int32 Add2ndX   = 1 << (PASS1_BITS - 2);

  int32 E [4], O [4]; //level 1 even and odd
  int32 EE[2], EO[2]; //level 2 even and odd
  int16 Tmp[8][8];    //partial transformed

  for (int32 j=0; j<8; j++) //horizontal transform
  {
    // E and O
    for(int32 k=0; k < 4; k++)
    {
      E[k] = Src[k] + Src[7 - k];
      O[k] = Src[k] - Src[7 - k];
    }
    Src += 8;

    // EE and EO
    EE[0] = E[0] + E[3];
    EO[0] = E[0] - E[3];
    EE[1] = E[1] + E[2];
    EO[1] = E[1] - E[2];

    //even part
    Tmp[0][j] = (int16)((EE[0] + EE[1]) << PASS1_BITS);
    Tmp[4][j] = (int16)((EE[0] - EE[1]) << PASS1_BITS);

    int32 A = (EO[1] + EO[0]) * (int32)tTC::c_P0o541196100;
    Tmp[2][j] = (int16)((A + (EO[0] * (int32)( tTC::c_P0o765366865)) + Add1st)>>Shift1st);
    Tmp[6][j] = (int16)((A + (EO[1] * (int32)(-tTC::c_P1o847759065)) + Add1st)>>Shift1st);

    //odd part
    int32 B  = (O[0] + O[1] + O[2] + O[3]) * tTC::c_P1o175875602;
    int32 B1 = (O[3] + O[0]) * (int32)(-tTC::c_P0o899976223);
    int32 B2 = (O[2] + O[1]) * (int32)(-tTC::c_P2o562915447);
    int32 B3 = (O[3] + O[1]) * (int32)(-tTC::c_P1o961570560) + B;
    int32 B4 = (O[2] + O[0]) * (int32)(-tTC::c_P0o390180644) + B;

    Tmp[1][j] = (int16)((O[0] * (int32)(tTC::c_P1o501321110) + B1 + B4 + Add1st)>>Shift1st);
    Tmp[3][j] = (int16)((O[1] * (int32)(tTC::c_P3o072711026) + B2 + B3 + Add1st)>>Shift1st);
    Tmp[5][j] = (int16)((O[2] * (int32)(tTC::c_P2o053119869) + B2 + B4 + Add1st)>>Shift1st);
    Tmp[7][j] = (int16)((O[3] * (int32)(tTC::c_P0o298631336) + B1 + B3 + Add1st)>>Shift1st);    
  }

  for (int32 j=0; j<8; j++) //vertical transform
  {    
    // E and O
    for(int32 k=0; k < 4; k++)
    {
      E[k] = Tmp[j][k] + Tmp[j][7 - k];
      O[k] = Tmp[j][k] - Tmp[j][7 - k];
    }

    // EE and EO
    EE[0] = E[0] + E[3];    
    EO[0] = E[0] - E[3];
    EE[1] = E[1] + E[2];
    EO[1] = E[1] - E[2];

    //even part
    Dst[0*8] = (int16)((EE[0] + EE[1] + Add2ndX) >> (PASS1_BITS - 1));
    Dst[4*8] = (int16)((EE[0] - EE[1] + Add2ndX) >> (PASS1_BITS - 1));

    int32 A = (EO[1] + EO[0]) * (int32)tTC::c_P0o541196100;
    Dst[2*8] = (int16)((A + (EO[0] * (int32)( tTC::c_P0o765366865)) + Add2nd)>>Shift2nd);
    Dst[6*8] = (int16)((A + (EO[1] * (int32)(-tTC::c_P1o847759065)) + Add2nd)>>Shift2nd);

    //odd part
    int32 B  = (O[0] + O[1] + O[2] + O[3]) * tTC::c_P1o175875602;
    int32 B1 = (O[3] + O[0]) * (int32)(-tTC::c_P0o899976223);
    int32 B2 = (O[2] + O[1]) * (int32)(-tTC::c_P2o562915447);
    int32 B3 = (O[3] + O[1]) * (int32)(-tTC::c_P1o961570560) + B;
    int32 B4 = (O[2] + O[0]) * (int32)(-tTC::c_P0o390180644) + B;

    Dst[1*8] = (int16)((O[0] * (int32)(tTC::c_P1o501321110) + B1 + B4 + Add2nd)>>Shift2nd);
    Dst[3*8] = (int16)((O[1] * (int32)(tTC::c_P3o072711026) + B2 + B3 + Add2nd)>>Shift2nd);
    Dst[5*8] = (int16)((O[2] * (int32)(tTC::c_P2o053119869) + B2 + B4 + Add2nd)>>Shift2nd);
    Dst[7*8] = (int16)((O[3] * (int32)(tTC::c_P0o298631336) + B1 + B3 + Add2nd)>>Shift2nd);
    Dst++;
  }
}
void xTransformSTD::InvTransformDCT_8x8_BTF(uint16* restrict Dst, const int16* Src)
{
  const int32 Shift1st  = tTC::c_CoeffPrec_BTF - PASS1_BITS;
  const int32 Add1st    = 1<<(Shift1st-1); 
  const int32 Shift2nd  = tTC::c_CoeffPrec_BTF + PASS1_BITS + 3;
  const int32 Add2nd    = 1<<(Shift2nd-1); 
  int32 E [4], O [4];
  int32 EE[2], EO[2];
  int16 Tmp[8][8];
  
  for (int32 j=0; j<8; j++) //vertical transform
  {
    //coeffs
    int32 Coeff0 = Src[0*8];
    int32 Coeff1 = Src[1*8];
    int32 Coeff2 = Src[2*8];
    int32 Coeff3 = Src[3*8];
    int32 Coeff4 = Src[4*8];
    int32 Coeff5 = Src[5*8];
    int32 Coeff6 = Src[6*8];
    int32 Coeff7 = Src[7*8];
    Src += 1;    

    //even part
    int32 A  = (Coeff2 + Coeff6) * tTC::c_P0o541196100; //z1
    EE[0] = (Coeff0 + Coeff4) << tTC::c_CoeffPrec_BTF; //tmp0
    EE[1] = (Coeff0 - Coeff4) << tTC::c_CoeffPrec_BTF; //tmp3      
    EO[0] = A + (Coeff2 * ( tTC::c_P0o765366865)); //tmp3
    EO[1] = A + (Coeff6 * (-tTC::c_P1o847759065)); //tmp2

    E[0] = EE[0] + EO[0]; //tmp10
    E[1] = EE[1] + EO[1]; //tmp11   
    E[2] = EE[1] - EO[1]; //tmp12
    E[3] = EE[0] - EO[0]; //tmp13

    //odd part
    int32 B  = (Coeff1 + Coeff3 + Coeff5 + Coeff7) * tTC::c_P1o175875602; 
    int32 B0 = (Coeff7 + Coeff1) * (-tTC::c_P0o899976223); //z1
    int32 B1 = (Coeff5 + Coeff3) * (-tTC::c_P2o562915447); //z2
    int32 B2 = (Coeff7 + Coeff3) * (-tTC::c_P1o961570560) + B;
    int32 B3 = (Coeff5 + Coeff1) * (-tTC::c_P0o390180644) + B;

    O[3] = Coeff7 * tTC::c_P0o298631336 + B0 + B2;
    O[2] = Coeff5 * tTC::c_P2o053119869 + B1 + B3;
    O[1] = Coeff3 * tTC::c_P3o072711026 + B1 + B2;
    O[0] = Coeff1 * tTC::c_P1o501321110 + B0 + B3; //tmp3

    for(int32 k=0;k<4;k++)
    {
      Tmp[k  ][j] = (int16)((E[k] + O[k] + Add1st)>>Shift1st);
      Tmp[7-k][j] = (int16)((E[k] - O[k] + Add1st)>>Shift1st);
    }
  }
  for (int32 j=0; j<8; j++) //horizontal transform
  { 
    //coeffs
    int32 Coeff0 = Tmp[j][0];
    int32 Coeff1 = Tmp[j][1];
    int32 Coeff2 = Tmp[j][2];
    int32 Coeff3 = Tmp[j][3];
    int32 Coeff4 = Tmp[j][4];
    int32 Coeff5 = Tmp[j][5];
    int32 Coeff6 = Tmp[j][6];
    int32 Coeff7 = Tmp[j][7];

    //even part
    int32 A  = (Coeff2 + Coeff6) * tTC::c_P0o541196100;
    EE[0] = (Coeff0 + Coeff4) << tTC::c_CoeffPrec_BTF;
    EE[1] = (Coeff0 - Coeff4) << tTC::c_CoeffPrec_BTF;
    EO[0] = A + (Coeff2 * ( tTC::c_P0o765366865));
    EO[1] = A + (Coeff6 * (-tTC::c_P1o847759065));

    E[0] = EE[0] + EO[0];
    E[1] = EE[1] + EO[1];    
    E[2] = EE[1] - EO[1]; 
    E[3] = EE[0] - EO[0];

    //odd part
    int32 B  = (Coeff1 + Coeff3 + Coeff5 + Coeff7) * tTC::c_P1o175875602; 
    int32 B0 = (Coeff7 + Coeff1) * (-tTC::c_P0o899976223);
    int32 B1 = (Coeff5 + Coeff3) * (-tTC::c_P2o562915447);
    int32 B2 = (Coeff7 + Coeff3) * (-tTC::c_P1o961570560) + B;
    int32 B3 = (Coeff5 + Coeff1) * (-tTC::c_P0o390180644) + B;

    O[3] = Coeff7 * tTC::c_P0o298631336 + B0 + B2;
    O[2] = Coeff5 * tTC::c_P2o053119869 + B1 + B3;
    O[1] = Coeff3 * tTC::c_P3o072711026 + B1 + B2;
    O[0] = Coeff1 * tTC::c_P1o501321110 + B0 + B3;

    for(int32 k=0;k<4;k++)
    {
      Dst[k  ] = (uint16)xClipU16(xClipS16((E[k] + O[k] + Add2nd)>>Shift2nd));
      Dst[7-k] = (uint16)xClipU16(xClipS16((E[k] - O[k] + Add2nd)>>Shift2nd));
    }
    Dst+= 8;
  }
}

//=====================================================================================================================================================================================
// xTransformSSE
//=====================================================================================================================================================================================
#if X_SIMD_CAN_USE_SSE
void xTransformSSE::FwdTransformDCT_8x8_M16(int16* restrict Dst, const uint16* restrict Src)
{
  const __m128i Add1stV = _mm_set1_epi32(tTC::c_FrwAdd1st_16bit);
  const __m128i Add2ndV = _mm_set1_epi32(tTC::c_FrwAdd2nd_16bit);

  const __m128i xT8[8] = {
    _mm_setr_epi16(16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384),
    _mm_setr_epi16(22725,  19266,  12873,   4520,  -4520, -12873, -19266, -22725),
    _mm_setr_epi16(21407,   8867,  -8867, -21407, -21407,  -8867,   8867,  21407),
    _mm_setr_epi16(19266,  -4520, -22725, -12873,  12873,  22725,   4520, -19266),
    _mm_setr_epi16(16384, -16384, -16384,  16384,  16384, -16384, -16384,  16384),
    _mm_setr_epi16(12873, -22725,   4520,  19266, -19266,  -4520,  22725, -12873),
    _mm_setr_epi16( 8867, -21407,  21407,  -8867,  -8867,  21407, -21407,   8867),
    _mm_setr_epi16( 4520, -12873,  19266, -22725,  22725, -19266,  12873,  -4520)};

  __m128i TmpV[8];

  //load and horizontal transform
  for(int32 j=0; j<8; j++)
  {
    __m128i SrcV = _mm_loadu_si128((__m128i*)Src);
    Src += 8;

    __m128i Tr0V = _mm_madd_epi16(SrcV, xT8[0]);
    __m128i Tr1V = _mm_madd_epi16(SrcV, xT8[1]);
    __m128i Tr2V = _mm_madd_epi16(SrcV, xT8[2]);
    __m128i Tr3V = _mm_madd_epi16(SrcV, xT8[3]);
    __m128i Tr4V = _mm_madd_epi16(SrcV, xT8[4]);
    __m128i Tr5V = _mm_madd_epi16(SrcV, xT8[5]);
    __m128i Tr6V = _mm_madd_epi16(SrcV, xT8[6]);
    __m128i Tr7V = _mm_madd_epi16(SrcV, xT8[7]);

    __m128i Tr01V = _mm_hadd_epi32(Tr0V, Tr1V);
    __m128i Tr23V = _mm_hadd_epi32(Tr2V, Tr3V);
    __m128i Tr45V = _mm_hadd_epi32(Tr4V, Tr5V);
    __m128i Tr67V = _mm_hadd_epi32(Tr6V, Tr7V);

    __m128i Tr0123V = _mm_hadd_epi32(Tr01V, Tr23V);
    __m128i Tr4567V = _mm_hadd_epi32(Tr45V, Tr67V);

    Tr0123V = _mm_srai_epi32(_mm_add_epi32(Tr0123V, Add1stV), tTC::c_FrwShift1st_16bit);
    Tr4567V = _mm_srai_epi32(_mm_add_epi32(Tr4567V, Add1stV), tTC::c_FrwShift1st_16bit);

    TmpV[j] = _mm_packs_epi32(Tr0123V, Tr4567V); 
  } 

  //transpose  
  {
    __m128i TrA0V = _mm_unpacklo_epi16(TmpV[0], TmpV[4]);
    __m128i TrA1V = _mm_unpackhi_epi16(TmpV[0], TmpV[4]);
    __m128i TrA2V = _mm_unpacklo_epi16(TmpV[1], TmpV[5]);
    __m128i TrA3V = _mm_unpackhi_epi16(TmpV[1], TmpV[5]);
    __m128i TrA4V = _mm_unpacklo_epi16(TmpV[2], TmpV[6]);
    __m128i TrA5V = _mm_unpackhi_epi16(TmpV[2], TmpV[6]);
    __m128i TrA6V = _mm_unpacklo_epi16(TmpV[3], TmpV[7]);
    __m128i TrA7V = _mm_unpackhi_epi16(TmpV[3], TmpV[7]);

    __m128i TrB0V = _mm_unpacklo_epi16(TrA0V, TrA4V);
    __m128i TrB1V = _mm_unpackhi_epi16(TrA0V, TrA4V);
    __m128i TrB2V = _mm_unpacklo_epi16(TrA1V, TrA5V);
    __m128i TrB3V = _mm_unpackhi_epi16(TrA1V, TrA5V);
    __m128i TrB4V = _mm_unpacklo_epi16(TrA2V, TrA6V);
    __m128i TrB5V = _mm_unpackhi_epi16(TrA2V, TrA6V);
    __m128i TrB6V = _mm_unpacklo_epi16(TrA3V, TrA7V);
    __m128i TrB7V = _mm_unpackhi_epi16(TrA3V, TrA7V);

    TmpV[0] = _mm_unpacklo_epi16(TrB0V, TrB4V);
    TmpV[1] = _mm_unpackhi_epi16(TrB0V, TrB4V);
    TmpV[2] = _mm_unpacklo_epi16(TrB1V, TrB5V);
    TmpV[3] = _mm_unpackhi_epi16(TrB1V, TrB5V);
    TmpV[4] = _mm_unpacklo_epi16(TrB2V, TrB6V);
    TmpV[5] = _mm_unpackhi_epi16(TrB2V, TrB6V);
    TmpV[6] = _mm_unpacklo_epi16(TrB3V, TrB7V);
    TmpV[7] = _mm_unpackhi_epi16(TrB3V, TrB7V);
  }  

  //vertical transform
  for(int32 j=0; j<8; j++)
  {
    __m128i SrcV = TmpV[j]; 

    __m128i Tr0V = _mm_madd_epi16(SrcV, xT8[0]);
    __m128i Tr1V = _mm_madd_epi16(SrcV, xT8[1]);
    __m128i Tr2V = _mm_madd_epi16(SrcV, xT8[2]);
    __m128i Tr3V = _mm_madd_epi16(SrcV, xT8[3]);
    __m128i Tr4V = _mm_madd_epi16(SrcV, xT8[4]);
    __m128i Tr5V = _mm_madd_epi16(SrcV, xT8[5]);
    __m128i Tr6V = _mm_madd_epi16(SrcV, xT8[6]);
    __m128i Tr7V = _mm_madd_epi16(SrcV, xT8[7]);

    __m128i Tr01V = _mm_hadd_epi32(Tr0V, Tr1V);
    __m128i Tr23V = _mm_hadd_epi32(Tr2V, Tr3V);
    __m128i Tr45V = _mm_hadd_epi32(Tr4V, Tr5V);
    __m128i Tr67V = _mm_hadd_epi32(Tr6V, Tr7V);

    __m128i Tr0123V = _mm_hadd_epi32(Tr01V, Tr23V);
    __m128i Tr4567V = _mm_hadd_epi32(Tr45V, Tr67V);

    Tr0123V = _mm_srai_epi32(_mm_add_epi32(Tr0123V, Add2ndV), tTC::c_FrwShift2nd_16bit);
    Tr4567V = _mm_srai_epi32(_mm_add_epi32(Tr4567V, Add2ndV), tTC::c_FrwShift2nd_16bit);

    TmpV[j] = _mm_packs_epi32(Tr0123V, Tr4567V); 
  }  

  //transpose  
  {
    __m128i TrA0V = _mm_unpacklo_epi16(TmpV[0], TmpV[4]);
    __m128i TrA1V = _mm_unpackhi_epi16(TmpV[0], TmpV[4]);
    __m128i TrA2V = _mm_unpacklo_epi16(TmpV[1], TmpV[5]);
    __m128i TrA3V = _mm_unpackhi_epi16(TmpV[1], TmpV[5]);
    __m128i TrA4V = _mm_unpacklo_epi16(TmpV[2], TmpV[6]);
    __m128i TrA5V = _mm_unpackhi_epi16(TmpV[2], TmpV[6]);
    __m128i TrA6V = _mm_unpacklo_epi16(TmpV[3], TmpV[7]);
    __m128i TrA7V = _mm_unpackhi_epi16(TmpV[3], TmpV[7]);

    __m128i TrB0V = _mm_unpacklo_epi16(TrA0V, TrA4V);
    __m128i TrB1V = _mm_unpackhi_epi16(TrA0V, TrA4V);
    __m128i TrB2V = _mm_unpacklo_epi16(TrA1V, TrA5V);
    __m128i TrB3V = _mm_unpackhi_epi16(TrA1V, TrA5V);
    __m128i TrB4V = _mm_unpacklo_epi16(TrA2V, TrA6V);
    __m128i TrB5V = _mm_unpackhi_epi16(TrA2V, TrA6V);
    __m128i TrB6V = _mm_unpacklo_epi16(TrA3V, TrA7V);
    __m128i TrB7V = _mm_unpackhi_epi16(TrA3V, TrA7V);

    TmpV[0] = _mm_unpacklo_epi16(TrB0V, TrB4V);
    TmpV[1] = _mm_unpackhi_epi16(TrB0V, TrB4V);
    TmpV[2] = _mm_unpacklo_epi16(TrB1V, TrB5V);
    TmpV[3] = _mm_unpackhi_epi16(TrB1V, TrB5V);
    TmpV[4] = _mm_unpacklo_epi16(TrB2V, TrB6V);
    TmpV[5] = _mm_unpackhi_epi16(TrB2V, TrB6V);
    TmpV[6] = _mm_unpacklo_epi16(TrB3V, TrB7V);
    TmpV[7] = _mm_unpackhi_epi16(TrB3V, TrB7V);
  }  

  //store
  for(int32 j=0; j<8; j++)
  {
    _mm_storeu_si128((__m128i*)Dst, TmpV[j]);
    Dst += 8;   
  }
}
void xTransformSSE::InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16* restrict Src)
{
  const __m128i Add1stV = _mm_set1_epi32(tTC::c_InvAdd1st_16bit);
  const __m128i Add2ndV = _mm_set1_epi32(tTC::c_InvAdd2nd_16bit);

  const __m128i xiT8[8] = {
    _mm_setr_epi16(16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520),
    _mm_setr_epi16(16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873),
    _mm_setr_epi16(16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266),
    _mm_setr_epi16(16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725),
    _mm_setr_epi16(16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725),
    _mm_setr_epi16(16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266),
    _mm_setr_epi16(16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873),
    _mm_setr_epi16(16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520)};

  __m128i TmpV[8];

  //load
  for(int32 j=0; j<8; j++)
  {
    TmpV[j] = _mm_loadu_si128((__m128i*)Src);
    Src += 8;   
  } 

  //transpose  
  {
    __m128i TrA0V = _mm_unpacklo_epi16(TmpV[0], TmpV[4]);
    __m128i TrA1V = _mm_unpackhi_epi16(TmpV[0], TmpV[4]);
    __m128i TrA2V = _mm_unpacklo_epi16(TmpV[1], TmpV[5]);
    __m128i TrA3V = _mm_unpackhi_epi16(TmpV[1], TmpV[5]);
    __m128i TrA4V = _mm_unpacklo_epi16(TmpV[2], TmpV[6]);
    __m128i TrA5V = _mm_unpackhi_epi16(TmpV[2], TmpV[6]);
    __m128i TrA6V = _mm_unpacklo_epi16(TmpV[3], TmpV[7]);
    __m128i TrA7V = _mm_unpackhi_epi16(TmpV[3], TmpV[7]);

    __m128i TrB0V = _mm_unpacklo_epi16(TrA0V, TrA4V);
    __m128i TrB1V = _mm_unpackhi_epi16(TrA0V, TrA4V);
    __m128i TrB2V = _mm_unpacklo_epi16(TrA1V, TrA5V);
    __m128i TrB3V = _mm_unpackhi_epi16(TrA1V, TrA5V);
    __m128i TrB4V = _mm_unpacklo_epi16(TrA2V, TrA6V);
    __m128i TrB5V = _mm_unpackhi_epi16(TrA2V, TrA6V);
    __m128i TrB6V = _mm_unpacklo_epi16(TrA3V, TrA7V);
    __m128i TrB7V = _mm_unpackhi_epi16(TrA3V, TrA7V);

    TmpV[0] = _mm_unpacklo_epi16(TrB0V, TrB4V);
    TmpV[1] = _mm_unpackhi_epi16(TrB0V, TrB4V);
    TmpV[2] = _mm_unpacklo_epi16(TrB1V, TrB5V);
    TmpV[3] = _mm_unpackhi_epi16(TrB1V, TrB5V);
    TmpV[4] = _mm_unpacklo_epi16(TrB2V, TrB6V);
    TmpV[5] = _mm_unpackhi_epi16(TrB2V, TrB6V);
    TmpV[6] = _mm_unpacklo_epi16(TrB3V, TrB7V);
    TmpV[7] = _mm_unpackhi_epi16(TrB3V, TrB7V);
  }  

  //vertical transform
  for(int32 j=0; j<8; j++)
  {
    __m128i SrcV = TmpV[j];

    __m128i Tr0V = _mm_madd_epi16(SrcV, xiT8[0]);
    __m128i Tr1V = _mm_madd_epi16(SrcV, xiT8[1]);
    __m128i Tr2V = _mm_madd_epi16(SrcV, xiT8[2]);
    __m128i Tr3V = _mm_madd_epi16(SrcV, xiT8[3]);
    __m128i Tr4V = _mm_madd_epi16(SrcV, xiT8[4]);
    __m128i Tr5V = _mm_madd_epi16(SrcV, xiT8[5]);
    __m128i Tr6V = _mm_madd_epi16(SrcV, xiT8[6]);
    __m128i Tr7V = _mm_madd_epi16(SrcV, xiT8[7]);

    __m128i Tr01V = _mm_hadd_epi32(Tr0V, Tr1V);
    __m128i Tr23V = _mm_hadd_epi32(Tr2V, Tr3V);
    __m128i Tr45V = _mm_hadd_epi32(Tr4V, Tr5V);
    __m128i Tr67V = _mm_hadd_epi32(Tr6V, Tr7V);

    __m128i Tr0123V = _mm_hadd_epi32(Tr01V, Tr23V);
    __m128i Tr4567V = _mm_hadd_epi32(Tr45V, Tr67V);

    Tr0123V = _mm_add_epi32(Tr0123V, Add1stV);
    Tr0123V = _mm_srai_epi32(Tr0123V, tTC::c_InvShift1st_16bit);

    Tr4567V = _mm_add_epi32(Tr4567V, Add1stV);
    Tr4567V = _mm_srai_epi32(Tr4567V, tTC::c_InvShift1st_16bit);

    TmpV[j] = _mm_packs_epi32(Tr0123V, Tr4567V); 
  } 

  //transpose  
  {
    __m128i TrA0V = _mm_unpacklo_epi16(TmpV[0], TmpV[4]);
    __m128i TrA1V = _mm_unpackhi_epi16(TmpV[0], TmpV[4]);
    __m128i TrA2V = _mm_unpacklo_epi16(TmpV[1], TmpV[5]);
    __m128i TrA3V = _mm_unpackhi_epi16(TmpV[1], TmpV[5]);
    __m128i TrA4V = _mm_unpacklo_epi16(TmpV[2], TmpV[6]);
    __m128i TrA5V = _mm_unpackhi_epi16(TmpV[2], TmpV[6]);
    __m128i TrA6V = _mm_unpacklo_epi16(TmpV[3], TmpV[7]);
    __m128i TrA7V = _mm_unpackhi_epi16(TmpV[3], TmpV[7]);

    __m128i TrB0V = _mm_unpacklo_epi16(TrA0V, TrA4V);
    __m128i TrB1V = _mm_unpackhi_epi16(TrA0V, TrA4V);
    __m128i TrB2V = _mm_unpacklo_epi16(TrA1V, TrA5V);
    __m128i TrB3V = _mm_unpackhi_epi16(TrA1V, TrA5V);
    __m128i TrB4V = _mm_unpacklo_epi16(TrA2V, TrA6V);
    __m128i TrB5V = _mm_unpackhi_epi16(TrA2V, TrA6V);
    __m128i TrB6V = _mm_unpacklo_epi16(TrA3V, TrA7V);
    __m128i TrB7V = _mm_unpackhi_epi16(TrA3V, TrA7V);

    TmpV[0] = _mm_unpacklo_epi16(TrB0V, TrB4V);
    TmpV[1] = _mm_unpackhi_epi16(TrB0V, TrB4V);
    TmpV[2] = _mm_unpacklo_epi16(TrB1V, TrB5V);
    TmpV[3] = _mm_unpackhi_epi16(TrB1V, TrB5V);
    TmpV[4] = _mm_unpacklo_epi16(TrB2V, TrB6V);
    TmpV[5] = _mm_unpackhi_epi16(TrB2V, TrB6V);
    TmpV[6] = _mm_unpacklo_epi16(TrB3V, TrB7V);
    TmpV[7] = _mm_unpackhi_epi16(TrB3V, TrB7V);
  }  

  //horizontal transform and store
  for(int32 j=0; j<8; j++)
  {
    __m128i SrcV = TmpV[j];    

    __m128i Tr0V = _mm_madd_epi16(SrcV, xiT8[0]);
    __m128i Tr1V = _mm_madd_epi16(SrcV, xiT8[1]);
    __m128i Tr2V = _mm_madd_epi16(SrcV, xiT8[2]);
    __m128i Tr3V = _mm_madd_epi16(SrcV, xiT8[3]);
    __m128i Tr4V = _mm_madd_epi16(SrcV, xiT8[4]);
    __m128i Tr5V = _mm_madd_epi16(SrcV, xiT8[5]);
    __m128i Tr6V = _mm_madd_epi16(SrcV, xiT8[6]);
    __m128i Tr7V = _mm_madd_epi16(SrcV, xiT8[7]);

    __m128i Tr01V = _mm_hadd_epi32(Tr0V, Tr1V);
    __m128i Tr23V = _mm_hadd_epi32(Tr2V, Tr3V);
    __m128i Tr45V = _mm_hadd_epi32(Tr4V, Tr5V);
    __m128i Tr67V = _mm_hadd_epi32(Tr6V, Tr7V);

    __m128i Tr0123V = _mm_hadd_epi32(Tr01V, Tr23V);
    __m128i Tr4567V = _mm_hadd_epi32(Tr45V, Tr67V);

    Tr0123V = _mm_add_epi32(Tr0123V, Add2ndV);
    Tr0123V = _mm_srai_epi32(Tr0123V, tTC::c_InvShift2nd_16bit);

    Tr4567V = _mm_add_epi32(Tr4567V, Add2ndV);
    Tr4567V = _mm_srai_epi32(Tr4567V, tTC::c_InvShift2nd_16bit);

    __m128i TrV = _mm_packus_epi32(Tr0123V, Tr4567V); 
    _mm_storeu_si128((__m128i*)Dst, TrV);
    Dst += 8; 
  }
}
#endif //X_SIMD_CAN_USE_SSE

//=====================================================================================================================================================================================
// xTransformAVX
//=====================================================================================================================================================================================
#if X_SIMD_CAN_USE_AVX
void xTransformAVX::FwdTransformDCT_8x8_M16(int16* restrict Dst, const uint16* Src)
{
  const __m256i Add1stV  = _mm256_set1_epi32(tTC::c_FrwAdd1st_16bit);
  const __m256i Add2ndV  = _mm256_set1_epi32(tTC::c_FrwAdd2nd_16bit);
  const __m256i PermCtlV = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);

  const __m256i xTC0_V = _mm256_setr_epi16(16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384,    16384, 16384, 16384, 16384, 16384, 16384, 16384, 16384);
  const __m256i xTC1_V = _mm256_setr_epi16(22725, 19266, 12873,  4520, -4520,-12873,-19266,-22725,    22725, 19266, 12873,  4520, -4520,-12873,-19266,-22725);
  const __m256i xTC2_V = _mm256_setr_epi16(21407,  8867, -8867,-21407,-21407, -8867,  8867, 21407,    21407,  8867, -8867,-21407,-21407, -8867,  8867, 21407);
  const __m256i xTC3_V = _mm256_setr_epi16(19266, -4520,-22725,-12873, 12873, 22725,  4520,-19266,    19266, -4520,-22725,-12873, 12873, 22725,  4520,-19266);
  const __m256i xTC4_V = _mm256_setr_epi16(16384,-16384,-16384, 16384, 16384,-16384,-16384, 16384,    16384,-16384,-16384, 16384, 16384,-16384,-16384, 16384);
  const __m256i xTC5_V = _mm256_setr_epi16(12873,-22725,  4520, 19266,-19266, -4520, 22725,-12873,    12873,-22725,  4520, 19266,-19266, -4520, 22725,-12873);
  const __m256i xTC6_V = _mm256_setr_epi16( 8867,-21407, 21407, -8867, -8867, 21407,-21407,  8867,     8867,-21407, 21407, -8867, -8867, 21407,-21407,  8867);
  const __m256i xTC7_V = _mm256_setr_epi16( 4520,-12873, 19266,-22725, 22725,-19266, 12873, -4520,     4520,-12873, 19266,-22725, 22725,-19266, 12873, -4520);

  //load
  __m256i Src_U16_V0 = _mm256_loadu_si256((__m256i*)(Src     ));
  __m256i Src_U16_V1 = _mm256_loadu_si256((__m256i*)(Src + 16));
  __m256i Src_U16_V2 = _mm256_loadu_si256((__m256i*)(Src + 32));
  __m256i Src_U16_V3 = _mm256_loadu_si256((__m256i*)(Src + 48));

  //horizontal transform
  __m256i TrH0_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC0_V), _mm256_madd_epi16(Src_U16_V1, xTC0_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC0_V), _mm256_madd_epi16(Src_U16_V3, xTC0_V)));
  __m256i TrH1_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC1_V), _mm256_madd_epi16(Src_U16_V1, xTC1_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC1_V), _mm256_madd_epi16(Src_U16_V3, xTC1_V)));
  __m256i TrH2_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC2_V), _mm256_madd_epi16(Src_U16_V1, xTC2_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC2_V), _mm256_madd_epi16(Src_U16_V3, xTC2_V)));
  __m256i TrH3_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC3_V), _mm256_madd_epi16(Src_U16_V1, xTC3_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC3_V), _mm256_madd_epi16(Src_U16_V3, xTC3_V)));
  __m256i TrH4_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC4_V), _mm256_madd_epi16(Src_U16_V1, xTC4_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC4_V), _mm256_madd_epi16(Src_U16_V3, xTC4_V)));
  __m256i TrH5_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC5_V), _mm256_madd_epi16(Src_U16_V1, xTC5_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC5_V), _mm256_madd_epi16(Src_U16_V3, xTC5_V)));
  __m256i TrH6_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC6_V), _mm256_madd_epi16(Src_U16_V1, xTC6_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC6_V), _mm256_madd_epi16(Src_U16_V3, xTC6_V)));
  __m256i TrH7_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V0, xTC7_V), _mm256_madd_epi16(Src_U16_V1, xTC7_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Src_U16_V2, xTC7_V), _mm256_madd_epi16(Src_U16_V3, xTC7_V)));

  __m256i TrsH0_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH0_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH1_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH1_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH2_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH2_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH3_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH3_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH4_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH4_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH5_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH5_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH6_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH6_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);
  __m256i TrsH7_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH7_I32_V, Add1stV), tTC::c_FrwShift1st_16bit), PermCtlV);

  __m256i Tri_I16_V0 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH0_I32_V, TrsH1_I32_V), 0xD8);
  __m256i Tri_I16_V1 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH2_I32_V, TrsH3_I32_V), 0xD8);
  __m256i Tri_I16_V2 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH4_I32_V, TrsH5_I32_V), 0xD8);
  __m256i Tri_I16_V3 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH6_I32_V, TrsH7_I32_V), 0xD8);

  //vertical transform
  __m256i TrV0_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC0_V), _mm256_madd_epi16(Tri_I16_V1, xTC0_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC0_V), _mm256_madd_epi16(Tri_I16_V3, xTC0_V)));
  __m256i TrV1_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC1_V), _mm256_madd_epi16(Tri_I16_V1, xTC1_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC1_V), _mm256_madd_epi16(Tri_I16_V3, xTC1_V)));
  __m256i TrV2_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC2_V), _mm256_madd_epi16(Tri_I16_V1, xTC2_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC2_V), _mm256_madd_epi16(Tri_I16_V3, xTC2_V)));
  __m256i TrV3_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC3_V), _mm256_madd_epi16(Tri_I16_V1, xTC3_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC3_V), _mm256_madd_epi16(Tri_I16_V3, xTC3_V)));
  __m256i TrV4_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC4_V), _mm256_madd_epi16(Tri_I16_V1, xTC4_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC4_V), _mm256_madd_epi16(Tri_I16_V3, xTC4_V)));
  __m256i TrV5_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC5_V), _mm256_madd_epi16(Tri_I16_V1, xTC5_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC5_V), _mm256_madd_epi16(Tri_I16_V3, xTC5_V)));
  __m256i TrV6_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC6_V), _mm256_madd_epi16(Tri_I16_V1, xTC6_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC6_V), _mm256_madd_epi16(Tri_I16_V3, xTC6_V)));
  __m256i TrV7_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC7_V), _mm256_madd_epi16(Tri_I16_V1, xTC7_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC7_V), _mm256_madd_epi16(Tri_I16_V3, xTC7_V)));

  __m256i TrsV0_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV0_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV1_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV1_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV2_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV2_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV3_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV3_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV4_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV4_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV5_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV5_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV6_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV6_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);
  __m256i TrsV7_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV7_I32_V, Add2ndV), tTC::c_FrwShift2nd_16bit), PermCtlV);

  __m256i Dst_I16_V0 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsV0_I32_V, TrsV1_I32_V), 0xD8);
  __m256i Dst_I16_V1 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsV2_I32_V, TrsV3_I32_V), 0xD8);
  __m256i Dst_I16_V2 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsV4_I32_V, TrsV5_I32_V), 0xD8);
  __m256i Dst_I16_V3 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsV6_I32_V, TrsV7_I32_V), 0xD8);

  //store
  _mm256_storeu_si256((__m256i*)(Dst     ), Dst_I16_V0);
  _mm256_storeu_si256((__m256i*)(Dst + 16), Dst_I16_V1);
  _mm256_storeu_si256((__m256i*)(Dst + 32), Dst_I16_V2);
  _mm256_storeu_si256((__m256i*)(Dst + 48), Dst_I16_V3);
}
void xTransformAVX::InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16* Src)
{
  const __m256i Add1stV  = _mm256_set1_epi32(tTC::c_InvAdd1st_16bit);
  const __m256i Add2ndV  = _mm256_set1_epi32(tTC::c_InvAdd2nd_16bit);
  const __m256i PermCtlV = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);

  const __m256i xTC0_V = _mm256_setr_epi16(16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,   16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520);
  const __m256i xTC1_V = _mm256_setr_epi16(16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873,   16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873);
  const __m256i xTC2_V = _mm256_setr_epi16(16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266,   16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266);
  const __m256i xTC3_V = _mm256_setr_epi16(16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725,   16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725);
  const __m256i xTC4_V = _mm256_setr_epi16(16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725,   16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725);
  const __m256i xTC5_V = _mm256_setr_epi16(16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266,   16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266);
  const __m256i xTC6_V = _mm256_setr_epi16(16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873,   16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873);
  const __m256i xTC7_V = _mm256_setr_epi16(16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520,   16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520);

  //load
  __m256i Src_I16_V0 = _mm256_loadu_si256((__m256i*)(Src     ));
  __m256i Src_I16_V1 = _mm256_loadu_si256((__m256i*)(Src + 16));
  __m256i Src_I16_V2 = _mm256_loadu_si256((__m256i*)(Src + 32));
  __m256i Src_I16_V3 = _mm256_loadu_si256((__m256i*)(Src + 48));

  //transpose  
  __m256i SrcT_I16_A0 = _mm256_permute4x64_epi64(Src_I16_V0, 0xD8);
  __m256i SrcT_I16_A1 = _mm256_permute4x64_epi64(Src_I16_V1, 0xD8);
  __m256i SrcT_I16_A2 = _mm256_permute4x64_epi64(Src_I16_V2, 0xD8);
  __m256i SrcT_I16_A3 = _mm256_permute4x64_epi64(Src_I16_V3, 0xD8);

  __m256i SrcT_I16_B0 = _mm256_unpacklo_epi16(SrcT_I16_A0, SrcT_I16_A2);
  __m256i SrcT_I16_B1 = _mm256_unpackhi_epi16(SrcT_I16_A0, SrcT_I16_A2);
  __m256i SrcT_I16_B2 = _mm256_unpacklo_epi16(SrcT_I16_A1, SrcT_I16_A3);
  __m256i SrcT_I16_B3 = _mm256_unpackhi_epi16(SrcT_I16_A1, SrcT_I16_A3);

  __m256i SrcT_I16_C0 = _mm256_unpacklo_epi16(SrcT_I16_B0, SrcT_I16_B2);
  __m256i SrcT_I16_C1 = _mm256_unpackhi_epi16(SrcT_I16_B0, SrcT_I16_B2);
  __m256i SrcT_I16_C2 = _mm256_unpacklo_epi16(SrcT_I16_B1, SrcT_I16_B3);
  __m256i SrcT_I16_C3 = _mm256_unpackhi_epi16(SrcT_I16_B1, SrcT_I16_B3);

  __m256i SrcT_I16_D0 = _mm256_unpacklo_epi16(SrcT_I16_C0, SrcT_I16_C2);
  __m256i SrcT_I16_D1 = _mm256_unpackhi_epi16(SrcT_I16_C0, SrcT_I16_C2);
  __m256i SrcT_I16_D2 = _mm256_unpacklo_epi16(SrcT_I16_C1, SrcT_I16_C3);
  __m256i SrcT_I16_D3 = _mm256_unpackhi_epi16(SrcT_I16_C1, SrcT_I16_C3);

  __m256i SrcT_I16_V0 = _mm256_permute2x128_si256(SrcT_I16_D0, SrcT_I16_D1, 0x20);
  __m256i SrcT_I16_V1 = _mm256_permute2x128_si256(SrcT_I16_D2, SrcT_I16_D3, 0x20);
  __m256i SrcT_I16_V2 = _mm256_permute2x128_si256(SrcT_I16_D0, SrcT_I16_D1, 0x31);
  __m256i SrcT_I16_V3 = _mm256_permute2x128_si256(SrcT_I16_D2, SrcT_I16_D3, 0x31);

  //horizontal transform
  __m256i TrH0_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC0_V), _mm256_madd_epi16(SrcT_I16_V1, xTC0_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC0_V), _mm256_madd_epi16(SrcT_I16_V3, xTC0_V)));
  __m256i TrH1_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC1_V), _mm256_madd_epi16(SrcT_I16_V1, xTC1_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC1_V), _mm256_madd_epi16(SrcT_I16_V3, xTC1_V)));
  __m256i TrH2_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC2_V), _mm256_madd_epi16(SrcT_I16_V1, xTC2_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC2_V), _mm256_madd_epi16(SrcT_I16_V3, xTC2_V)));
  __m256i TrH3_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC3_V), _mm256_madd_epi16(SrcT_I16_V1, xTC3_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC3_V), _mm256_madd_epi16(SrcT_I16_V3, xTC3_V)));
  __m256i TrH4_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC4_V), _mm256_madd_epi16(SrcT_I16_V1, xTC4_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC4_V), _mm256_madd_epi16(SrcT_I16_V3, xTC4_V)));
  __m256i TrH5_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC5_V), _mm256_madd_epi16(SrcT_I16_V1, xTC5_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC5_V), _mm256_madd_epi16(SrcT_I16_V3, xTC5_V)));
  __m256i TrH6_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC6_V), _mm256_madd_epi16(SrcT_I16_V1, xTC6_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC6_V), _mm256_madd_epi16(SrcT_I16_V3, xTC6_V)));
  __m256i TrH7_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V0, xTC7_V), _mm256_madd_epi16(SrcT_I16_V1, xTC7_V)), _mm256_hadd_epi32(_mm256_madd_epi16(SrcT_I16_V2, xTC7_V), _mm256_madd_epi16(SrcT_I16_V3, xTC7_V)));

  __m256i TrsH0_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH0_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH1_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH1_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH2_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH2_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH3_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH3_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH4_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH4_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH5_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH5_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH6_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH6_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);
  __m256i TrsH7_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrH7_I32_V, Add1stV), tTC::c_InvShift1st_16bit), PermCtlV);

  __m256i Tri_I16_V0 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH0_I32_V, TrsH1_I32_V), 0xD8);
  __m256i Tri_I16_V1 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH2_I32_V, TrsH3_I32_V), 0xD8);
  __m256i Tri_I16_V2 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH4_I32_V, TrsH5_I32_V), 0xD8);
  __m256i Tri_I16_V3 = _mm256_permute4x64_epi64(_mm256_packs_epi32(TrsH6_I32_V, TrsH7_I32_V), 0xD8);

  //vertical transform
  __m256i TrV0_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC0_V), _mm256_madd_epi16(Tri_I16_V1, xTC0_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC0_V), _mm256_madd_epi16(Tri_I16_V3, xTC0_V)));
  __m256i TrV1_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC1_V), _mm256_madd_epi16(Tri_I16_V1, xTC1_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC1_V), _mm256_madd_epi16(Tri_I16_V3, xTC1_V)));
  __m256i TrV2_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC2_V), _mm256_madd_epi16(Tri_I16_V1, xTC2_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC2_V), _mm256_madd_epi16(Tri_I16_V3, xTC2_V)));
  __m256i TrV3_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC3_V), _mm256_madd_epi16(Tri_I16_V1, xTC3_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC3_V), _mm256_madd_epi16(Tri_I16_V3, xTC3_V)));
  __m256i TrV4_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC4_V), _mm256_madd_epi16(Tri_I16_V1, xTC4_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC4_V), _mm256_madd_epi16(Tri_I16_V3, xTC4_V)));
  __m256i TrV5_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC5_V), _mm256_madd_epi16(Tri_I16_V1, xTC5_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC5_V), _mm256_madd_epi16(Tri_I16_V3, xTC5_V)));
  __m256i TrV6_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC6_V), _mm256_madd_epi16(Tri_I16_V1, xTC6_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC6_V), _mm256_madd_epi16(Tri_I16_V3, xTC6_V)));
  __m256i TrV7_I32_V = _mm256_hadd_epi32(_mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V0, xTC7_V), _mm256_madd_epi16(Tri_I16_V1, xTC7_V)), _mm256_hadd_epi32(_mm256_madd_epi16(Tri_I16_V2, xTC7_V), _mm256_madd_epi16(Tri_I16_V3, xTC7_V)));

  __m256i TrsV0_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV0_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV1_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV1_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV2_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV2_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV3_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV3_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV4_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV4_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV5_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV5_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV6_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV6_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);
  __m256i TrsV7_I32_V = _mm256_permutevar8x32_epi32(_mm256_srai_epi32(_mm256_add_epi32(TrV7_I32_V, Add2ndV), tTC::c_InvShift2nd_16bit), PermCtlV);

  __m256i Dst_U16_V0 = _mm256_permute4x64_epi64(_mm256_packus_epi32(TrsV0_I32_V, TrsV1_I32_V), 0xD8);
  __m256i Dst_U16_V1 = _mm256_permute4x64_epi64(_mm256_packus_epi32(TrsV2_I32_V, TrsV3_I32_V), 0xD8);
  __m256i Dst_U16_V2 = _mm256_permute4x64_epi64(_mm256_packus_epi32(TrsV4_I32_V, TrsV5_I32_V), 0xD8);
  __m256i Dst_U16_V3 = _mm256_permute4x64_epi64(_mm256_packus_epi32(TrsV6_I32_V, TrsV7_I32_V), 0xD8);

  //transpose  
  __m256i DstT_U16_A0 = _mm256_permute4x64_epi64(Dst_U16_V0, 0xD8);
  __m256i DstT_U16_A1 = _mm256_permute4x64_epi64(Dst_U16_V1, 0xD8);
  __m256i DstT_U16_A2 = _mm256_permute4x64_epi64(Dst_U16_V2, 0xD8);
  __m256i DstT_U16_A3 = _mm256_permute4x64_epi64(Dst_U16_V3, 0xD8);

  __m256i DstT_U16_B0 = _mm256_unpacklo_epi16(DstT_U16_A0, DstT_U16_A2);
  __m256i DstT_U16_B1 = _mm256_unpackhi_epi16(DstT_U16_A0, DstT_U16_A2);
  __m256i DstT_U16_B2 = _mm256_unpacklo_epi16(DstT_U16_A1, DstT_U16_A3);
  __m256i DstT_U16_B3 = _mm256_unpackhi_epi16(DstT_U16_A1, DstT_U16_A3);

  __m256i DstT_U16_C0 = _mm256_unpacklo_epi16(DstT_U16_B0, DstT_U16_B2);
  __m256i DstT_U16_C1 = _mm256_unpackhi_epi16(DstT_U16_B0, DstT_U16_B2);
  __m256i DstT_U16_C2 = _mm256_unpacklo_epi16(DstT_U16_B1, DstT_U16_B3);
  __m256i DstT_U16_C3 = _mm256_unpackhi_epi16(DstT_U16_B1, DstT_U16_B3);

  __m256i DstT_U16_D0 = _mm256_unpacklo_epi16(DstT_U16_C0, DstT_U16_C2);
  __m256i DstT_U16_D1 = _mm256_unpackhi_epi16(DstT_U16_C0, DstT_U16_C2);
  __m256i DstT_U16_D2 = _mm256_unpacklo_epi16(DstT_U16_C1, DstT_U16_C3);
  __m256i DstT_U16_D3 = _mm256_unpackhi_epi16(DstT_U16_C1, DstT_U16_C3);

  __m256i DstT_U16_V0 = _mm256_permute2x128_si256(DstT_U16_D0, DstT_U16_D1, 0x20);
  __m256i DstT_U16_V1 = _mm256_permute2x128_si256(DstT_U16_D2, DstT_U16_D3, 0x20);
  __m256i DstT_U16_V2 = _mm256_permute2x128_si256(DstT_U16_D0, DstT_U16_D1, 0x31);
  __m256i DstT_U16_V3 = _mm256_permute2x128_si256(DstT_U16_D2, DstT_U16_D3, 0x31);

  //store
  _mm256_storeu_si256((__m256i*)(Dst     ), DstT_U16_V0);
  _mm256_storeu_si256((__m256i*)(Dst + 16), DstT_U16_V1);
  _mm256_storeu_si256((__m256i*)(Dst + 32), DstT_U16_V2);
  _mm256_storeu_si256((__m256i*)(Dst + 48), DstT_U16_V3);
}
#endif //X_SIMD_CAN_USE_AVX

//=====================================================================================================================================================================================
// xTransformAVX512
//=====================================================================================================================================================================================
#if X_SIMD_CAN_USE_AVX512
void xTransformAVX512::FwdTransformDCT_8x8_M16(int16* restrict Dst, const uint16* Src)
{
  const __m512i Add1stV  = _mm512_set1_epi32(tTC::c_FrwAdd1st_16bit);
  const __m512i Add2ndV  = _mm512_set1_epi32(tTC::c_FrwAdd2nd_16bit);
  const __m512i PermCtlV = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

  const __m512i xTC0_V = _mm512_setr_epi16(16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,   16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,   16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384,   16384,  16384,  16384,  16384,  16384,  16384,  16384,  16384);
  const __m512i xTC1_V = _mm512_setr_epi16(22725,  19266,  12873,   4520,  -4520, -12873, -19266, -22725,   22725,  19266,  12873,   4520,  -4520, -12873, -19266, -22725,   22725,  19266,  12873,   4520,  -4520, -12873, -19266, -22725,   22725,  19266,  12873,   4520,  -4520, -12873, -19266, -22725);
  const __m512i xTC2_V = _mm512_setr_epi16(21407,   8867,  -8867, -21407, -21407,  -8867,   8867,  21407,   21407,   8867,  -8867, -21407, -21407,  -8867,   8867,  21407,   21407,   8867,  -8867, -21407, -21407,  -8867,   8867,  21407,   21407,   8867,  -8867, -21407, -21407,  -8867,   8867,  21407);
  const __m512i xTC3_V = _mm512_setr_epi16(19266,  -4520, -22725, -12873,  12873,  22725,   4520, -19266,   19266,  -4520, -22725, -12873,  12873,  22725,   4520, -19266,   19266,  -4520, -22725, -12873,  12873,  22725,   4520, -19266,   19266,  -4520, -22725, -12873,  12873,  22725,   4520, -19266);
  const __m512i xTC4_V = _mm512_setr_epi16(16384, -16384, -16384,  16384,  16384, -16384, -16384,  16384,   16384, -16384, -16384,  16384,  16384, -16384, -16384,  16384,   16384, -16384, -16384,  16384,  16384, -16384, -16384,  16384,   16384, -16384, -16384,  16384,  16384, -16384, -16384,  16384);
  const __m512i xTC5_V = _mm512_setr_epi16(12873, -22725,   4520,  19266, -19266,  -4520,  22725, -12873,   12873, -22725,   4520,  19266, -19266,  -4520,  22725, -12873,   12873, -22725,   4520,  19266, -19266,  -4520,  22725, -12873,   12873, -22725,   4520,  19266, -19266,  -4520,  22725, -12873);
  const __m512i xTC6_V = _mm512_setr_epi16( 8867, -21407,  21407,  -8867,  -8867,  21407, -21407,   8867,    8867, -21407,  21407,  -8867,  -8867,  21407, -21407,   8867,    8867, -21407,  21407,  -8867,  -8867,  21407, -21407,   8867,    8867, -21407,  21407,  -8867,  -8867,  21407, -21407,   8867);
  const __m512i xTC7_V = _mm512_setr_epi16( 4520, -12873,  19266, -22725,  22725, -19266,  12873,  -4520,    4520, -12873,  19266, -22725,  22725, -19266,  12873,  -4520,    4520, -12873,  19266, -22725,  22725, -19266,  12873,  -4520,    4520, -12873,  19266, -22725,  22725, -19266,  12873,  -4520);

  //load
  __m512i Src_U16_V0 = _mm512_loadu_si512((__m512i*)(Src     ));
  __m512i Src_U16_V1 = _mm512_loadu_si512((__m512i*)(Src + 32));

  //horizontal transform & transpose
  __m512i TrH0_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC0_V), _mm512_madd_epi16(Src_U16_V1, xTC0_V));
  __m512i TrH1_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC1_V), _mm512_madd_epi16(Src_U16_V1, xTC1_V));
  __m512i TrH2_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC2_V), _mm512_madd_epi16(Src_U16_V1, xTC2_V));
  __m512i TrH3_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC3_V), _mm512_madd_epi16(Src_U16_V1, xTC3_V));
  __m512i TrH4_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC4_V), _mm512_madd_epi16(Src_U16_V1, xTC4_V));
  __m512i TrH5_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC5_V), _mm512_madd_epi16(Src_U16_V1, xTC5_V));
  __m512i TrH6_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC6_V), _mm512_madd_epi16(Src_U16_V1, xTC6_V));
  __m512i TrH7_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Src_U16_V0, xTC7_V), _mm512_madd_epi16(Src_U16_V1, xTC7_V));

  __m512i TrH01_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH0_I32_V, TrH1_I32_V), Add1stV), tTC::c_FrwShift1st_16bit);
  __m512i TrH23_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH2_I32_V, TrH3_I32_V), Add1stV), tTC::c_FrwShift1st_16bit);
  __m512i TrH45_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH4_I32_V, TrH5_I32_V), Add1stV), tTC::c_FrwShift1st_16bit);
  __m512i TrH67_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH6_I32_V, TrH7_I32_V), Add1stV), tTC::c_FrwShift1st_16bit);

  __m512i Tri_I16_V0 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packs_epi32(TrH01_I32_V, TrH23_I32_V));
  __m512i Tri_I16_V1 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packs_epi32(TrH45_I32_V, TrH67_I32_V));

  //vertical transform & transpose
  __m512i TrV0_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC0_V), _mm512_madd_epi16(Tri_I16_V1, xTC0_V));
  __m512i TrV1_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC1_V), _mm512_madd_epi16(Tri_I16_V1, xTC1_V));
  __m512i TrV2_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC2_V), _mm512_madd_epi16(Tri_I16_V1, xTC2_V));
  __m512i TrV3_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC3_V), _mm512_madd_epi16(Tri_I16_V1, xTC3_V));
  __m512i TrV4_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC4_V), _mm512_madd_epi16(Tri_I16_V1, xTC4_V));
  __m512i TrV5_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC5_V), _mm512_madd_epi16(Tri_I16_V1, xTC5_V));
  __m512i TrV6_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC6_V), _mm512_madd_epi16(Tri_I16_V1, xTC6_V));
  __m512i TrV7_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC7_V), _mm512_madd_epi16(Tri_I16_V1, xTC7_V));

  __m512i TrV01_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV0_I32_V, TrV1_I32_V), Add2ndV), tTC::c_FrwShift2nd_16bit);
  __m512i TrV23_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV2_I32_V, TrV3_I32_V), Add2ndV), tTC::c_FrwShift2nd_16bit);
  __m512i TrV45_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV4_I32_V, TrV5_I32_V), Add2ndV), tTC::c_FrwShift2nd_16bit);
  __m512i TrV67_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV6_I32_V, TrV7_I32_V), Add2ndV), tTC::c_FrwShift2nd_16bit);

  __m512i Dst_I16_V0 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packs_epi32(TrV01_I32_V, TrV23_I32_V));
  __m512i Dst_I16_V1 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packs_epi32(TrV45_I32_V, TrV67_I32_V));

  //store
  _mm512_storeu_si512((__m512i*)(Dst     ), Dst_I16_V0);
  _mm512_storeu_si512((__m512i*)(Dst + 32), Dst_I16_V1);
}
void xTransformAVX512::InvTransformDCT_8x8_M16(uint16* restrict Dst, const int16* Src)
{
  const __m512i Add1stV  = _mm512_set1_epi32(tTC::c_InvAdd1st_16bit);
  const __m512i Add2ndV  = _mm512_set1_epi32(tTC::c_InvAdd2nd_16bit);
  const __m512i PermCtlV = _mm512_setr_epi64(0, 2, 4, 6, 1, 3, 5, 7);

  const __m512i Transpose_U16_V0  = _mm512_setr_epi16(0, 8,16,24,32,40,48,56,
                                                      1, 9,17,25,33,41,49,57,
                                                      2,10,18,26,34,42,50,58,
                                                      3,11,19,27,35,43,51,59);
  const __m512i Transpose_U16_V1  = _mm512_setr_epi16(4,12,20,28,36,44,52,60,
                                                      5,13,21,29,37,45,53,61,
                                                      6,14,22,30,38,46,54,62,
                                                      7,15,23,31,39,47,55,63);

  const __m512i xTC0_V = _mm512_setr_epi16(16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,   16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,   16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520,   16384, 22725, 21407, 19266, 16384, 12873,  8867,  4520);
  const __m512i xTC1_V = _mm512_setr_epi16(16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873,   16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873,   16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873,   16384, 19266,  8867, -4520,-16384,-22725,-21407,-12873);
  const __m512i xTC2_V = _mm512_setr_epi16(16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266,   16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266,   16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266,   16384, 12873, -8867,-22725,-16384,  4520, 21407, 19266);
  const __m512i xTC3_V = _mm512_setr_epi16(16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725,   16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725,   16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725,   16384,  4520,-21407,-12873, 16384, 19266, -8867,-22725);
  const __m512i xTC4_V = _mm512_setr_epi16(16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725,   16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725,   16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725,   16384, -4520,-21407, 12873, 16384,-19266, -8867, 22725);
  const __m512i xTC5_V = _mm512_setr_epi16(16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266,   16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266,   16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266,   16384,-12873, -8867, 22725,-16384, -4520, 21407,-19266);
  const __m512i xTC6_V = _mm512_setr_epi16(16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873,   16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873,   16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873,   16384,-19266,  8867,  4520,-16384, 22725,-21407, 12873);
  const __m512i xTC7_V = _mm512_setr_epi16(16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520,   16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520,   16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520,   16384,-22725, 21407,-19266, 16384,-12873,  8867, -4520);

  //load
  __m512i Src_I16_V0 = _mm512_loadu_si512((__m512i*)(Src     ));
  __m512i Src_I16_V1 = _mm512_loadu_si512((__m512i*)(Src + 32));

  //transpose
  __m512i SrcT_I16_V0 = _mm512_permutex2var_epi16(Src_I16_V0, Transpose_U16_V0, Src_I16_V1);
  __m512i SrcT_I16_V1 = _mm512_permutex2var_epi16(Src_I16_V0, Transpose_U16_V1, Src_I16_V1);

  //horizontal transform & transpose
  __m512i TrH0_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC0_V), _mm512_madd_epi16(SrcT_I16_V1, xTC0_V));
  __m512i TrH1_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC1_V), _mm512_madd_epi16(SrcT_I16_V1, xTC1_V));
  __m512i TrH2_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC2_V), _mm512_madd_epi16(SrcT_I16_V1, xTC2_V));
  __m512i TrH3_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC3_V), _mm512_madd_epi16(SrcT_I16_V1, xTC3_V));
  __m512i TrH4_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC4_V), _mm512_madd_epi16(SrcT_I16_V1, xTC4_V));
  __m512i TrH5_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC5_V), _mm512_madd_epi16(SrcT_I16_V1, xTC5_V));
  __m512i TrH6_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC6_V), _mm512_madd_epi16(SrcT_I16_V1, xTC6_V));
  __m512i TrH7_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(SrcT_I16_V0, xTC7_V), _mm512_madd_epi16(SrcT_I16_V1, xTC7_V));

  __m512i TrH01_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH0_I32_V, TrH1_I32_V), Add1stV), tTC::c_InvShift1st_16bit);
  __m512i TrH23_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH2_I32_V, TrH3_I32_V), Add1stV), tTC::c_InvShift1st_16bit);
  __m512i TrH45_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH4_I32_V, TrH5_I32_V), Add1stV), tTC::c_InvShift1st_16bit);
  __m512i TrH67_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrH6_I32_V, TrH7_I32_V), Add1stV), tTC::c_InvShift1st_16bit);

  __m512i Tri_I16_V0 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packs_epi32(TrH01_I32_V, TrH23_I32_V));
  __m512i Tri_I16_V1 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packs_epi32(TrH45_I32_V, TrH67_I32_V));

  //vertical transform & transpose
  __m512i TrV0_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC0_V), _mm512_madd_epi16(Tri_I16_V1, xTC0_V));
  __m512i TrV1_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC1_V), _mm512_madd_epi16(Tri_I16_V1, xTC1_V));
  __m512i TrV2_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC2_V), _mm512_madd_epi16(Tri_I16_V1, xTC2_V));
  __m512i TrV3_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC3_V), _mm512_madd_epi16(Tri_I16_V1, xTC3_V));
  __m512i TrV4_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC4_V), _mm512_madd_epi16(Tri_I16_V1, xTC4_V));
  __m512i TrV5_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC5_V), _mm512_madd_epi16(Tri_I16_V1, xTC5_V));
  __m512i TrV6_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC6_V), _mm512_madd_epi16(Tri_I16_V1, xTC6_V));
  __m512i TrV7_I32_V = _mm512_hadd_epi32(_mm512_madd_epi16(Tri_I16_V0, xTC7_V), _mm512_madd_epi16(Tri_I16_V1, xTC7_V));

  __m512i TrV01_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV0_I32_V, TrV1_I32_V), Add2ndV), tTC::c_InvShift2nd_16bit);
  __m512i TrV23_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV2_I32_V, TrV3_I32_V), Add2ndV), tTC::c_InvShift2nd_16bit);
  __m512i TrV45_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV4_I32_V, TrV5_I32_V), Add2ndV), tTC::c_InvShift2nd_16bit);
  __m512i TrV67_I32_V = _mm512_srai_epi32(_mm512_add_epi32(_mm512_hadd_epi32(TrV6_I32_V, TrV7_I32_V), Add2ndV), tTC::c_InvShift2nd_16bit);

  __m512i Dst_U16_V0 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packus_epi32(TrV01_I32_V, TrV23_I32_V));
  __m512i Dst_U16_V1 = _mm512_permutexvar_epi64(PermCtlV, _mm512_packus_epi32(TrV45_I32_V, TrV67_I32_V));

  ////transpose
  __m512i DstT_U16_V0 = _mm512_permutex2var_epi16(Dst_U16_V0, Transpose_U16_V0, Dst_U16_V1);
  __m512i DstT_U16_V1 = _mm512_permutex2var_epi16(Dst_U16_V0, Transpose_U16_V1, Dst_U16_V1);

  //store
  _mm512_storeu_si512((__m512i*)(Dst     ), DstT_U16_V0);
  _mm512_storeu_si512((__m512i*)(Dst + 32), DstT_U16_V1);
}
#endif //X_SIMD_CAN_USE_AVX512

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG