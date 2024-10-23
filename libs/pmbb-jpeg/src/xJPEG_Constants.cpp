/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Constants.h"

namespace PMBB_NAMESPACE::JPEG {

//===============================================================================================================================================================================================================
// Forvard and inverse zig-zag scan
//===============================================================================================================================================================================================================
//const uint8 xJPEG_Constants::m_ScanZigZag[64];
const uint8 xJPEG_Constants::m_InvScanZigZag[64] =
{
   0,  1,  5,  6, 14, 15, 27, 28,
   2,  4,  7, 13, 16, 26, 29, 42,
   3,  8, 12, 17, 25, 30, 41, 43,
   9, 11, 18, 24, 31, 40, 44, 53,
  10, 19, 23, 32, 39, 45, 52, 54,
  20, 22, 33, 38, 46, 51, 55, 60,
  21, 34, 37, 47, 50, 56, 59, 61,
  35, 36, 48, 49, 57, 58, 62, 63,
};

//===============================================================================================================================================================================================================
// Default quantization tables in raster order
//===============================================================================================================================================================================================================
const uint8 xJPEG_Constants::m_QuantTabDefLumaR[64] =
{
  16, 11, 10, 16, 24, 40, 51, 61,
  12, 12, 14, 19, 26, 58, 60, 55,
  14, 13, 16, 24, 40, 57, 69, 56,
  14, 17, 22, 29, 51, 87, 80, 62,
  18, 22, 37, 56, 68,109,103, 77,
  24, 35, 55, 64, 81,104,113, 92,
  49, 64, 78, 87,103,121,120,101,
  72, 92, 95, 98,112,100,103, 99,
};
const uint8 xJPEG_Constants::m_QuantTabDefChromaR[64] =
{
  17, 18, 24, 47, 99, 99, 99, 99,
  18, 21, 26, 66, 99, 99, 99, 99,
  24, 26, 56, 99, 99, 99, 99, 99,
  47, 66, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
};

//===============================================================================================================================================================================================================
// Default quantization tables in zig-zag order
//===============================================================================================================================================================================================================
const uint8 xJPEG_Constants::m_QuantTabDefLumaZ  [64] =
{
  16, 11, 12, 14, 12, 10, 16, 14, 
  13, 14, 18, 17, 16, 19, 24, 40,
  26, 24, 22, 22, 24, 49, 35, 37,
  29, 40, 58, 51, 61, 60, 57, 51,
  56, 55, 64, 72, 92, 78, 64, 68,
  87, 69, 55, 56, 80,109, 81, 87,
  95, 98,103,104,103, 62, 77,113,
 121,112,100,120, 92,101,103, 99,
};
const uint8 xJPEG_Constants::m_QuantTabDefChromaZ[64] =
{ 
  17, 18, 18, 24, 21, 24, 47, 26,
  26, 47, 99, 66, 56, 66, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99, 
};

//===============================================================================================================================================================================================================
// Default huffman tables
//===============================================================================================================================================================================================================
const uint8 xJPEG_Constants::m_CodeLengthLumaDC[] = { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
const uint8 xJPEG_Constants::m_CodeSymbolLumaDC[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
const uint8 xJPEG_Constants::m_CodeLengthLumaAC[] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
const uint8 xJPEG_Constants::m_CodeSymbolLumaAC[] =
{
  0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
  0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
  0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
  0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
  0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
  0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
  0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
  0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
  0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
  0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
  0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
  0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
  0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
  0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
  0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
  0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
  0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa,
};

const uint8 xJPEG_Constants::m_CodeLengthChromaDC[] = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
const uint8 xJPEG_Constants::m_CodeSymbolChromaDC[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
const uint8 xJPEG_Constants::m_CodeLengthChromaAC[] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
const uint8 xJPEG_Constants::m_CodeSymbolChromaAC[] =
{
  0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
  0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
  0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
  0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
  0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
  0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
  0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
  0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
  0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
  0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
  0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
  0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
  0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
  0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
  0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
  0xf9, 0xfa,
};
//===============================================================================================================================================================================================================
// Default huffman tables as codes used in encoder
//===============================================================================================================================================================================================================
const uint8  xJPEG_Constants::c_HuffLenDefaultLumaDC   [16 ] = { 2, 3, 3, 3, 3, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, };
const uint16 xJPEG_Constants::c_HuffCodeDefaultLumaDC  [16 ] = { 0x0, 0x2, 0x3, 0x4, 0x5, 0x6, 0xe, 0x1e, 0x3e, 0x7e, 0xfe, 0x1fe, 0x0, 0x0, 0x0, 0x0, };
const uint8  xJPEG_Constants::c_HuffLenDefaultLumaAC   [256] =
{
   4,  2,  2,  3,  4,  5,  7,  8, 10, 16, 16,  0,  0,  0,  0,  0, 
   0,  4,  5,  7,  9, 11, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  5,  8, 10, 12, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  6,  9, 12, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  6, 10, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  7, 11, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  7, 12, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  8, 12, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 15, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0, 10, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0, 10, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0, 11, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
  11, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
};
const uint16 xJPEG_Constants::c_HuffCodeDefaultLumaAC  [256] =
{//Bit=0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,     15,     
  0x000a, 0x0000, 0x0001, 0x0004, 0x000b, 0x001a, 0x0078, 0x00f8, 0x03f6, 0xff82, 0xff83, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 0
  0x0000, 0x000c, 0x001b, 0x0079, 0x01f6, 0x07f6, 0xff84, 0xff85, 0xff86, 0xff87, 0xff88, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 1
  0x0000, 0x001c, 0x00f9, 0x03f7, 0x0ff4, 0xff89, 0xff8a, 0xff8b, 0xff8c, 0xff8d, 0xff8e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 2
  0x0000, 0x003a, 0x01f7, 0x0ff5, 0xff8f, 0xff90, 0xff91, 0xff92, 0xff93, 0xff94, 0xff95, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 3
  0x0000, 0x003b, 0x03f8, 0xff96, 0xff97, 0xff98, 0xff99, 0xff9a, 0xff9b, 0xff9c, 0xff9d, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 4
  0x0000, 0x007a, 0x07f7, 0xff9e, 0xff9f, 0xffa0, 0xffa1, 0xffa2, 0xffa3, 0xffa4, 0xffa5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 5
  0x0000, 0x007b, 0x0ff6, 0xffa6, 0xffa7, 0xffa8, 0xffa9, 0xffaa, 0xffab, 0xffac, 0xffad, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 6
  0x0000, 0x00fa, 0x0ff7, 0xffae, 0xffaf, 0xffb0, 0xffb1, 0xffb2, 0xffb3, 0xffb4, 0xffb5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 7
  0x0000, 0x01f8, 0x7fc0, 0xffb6, 0xffb7, 0xffb8, 0xffb9, 0xffba, 0xffbb, 0xffbc, 0xffbd, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 8
  0x0000, 0x01f9, 0xffbe, 0xffbf, 0xffc0, 0xffc1, 0xffc2, 0xffc3, 0xffc4, 0xffc5, 0xffc6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 9
  0x0000, 0x01fa, 0xffc7, 0xffc8, 0xffc9, 0xffca, 0xffcb, 0xffcc, 0xffcd, 0xffce, 0xffcf, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 10
  0x0000, 0x03f9, 0xffd0, 0xffd1, 0xffd2, 0xffd3, 0xffd4, 0xffd5, 0xffd6, 0xffd7, 0xffd8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 11
  0x0000, 0x03fa, 0xffd9, 0xffda, 0xffdb, 0xffdc, 0xffdd, 0xffde, 0xffdf, 0xffe0, 0xffe1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 12
  0x0000, 0x07f8, 0xffe2, 0xffe3, 0xffe4, 0xffe5, 0xffe6, 0xffe7, 0xffe8, 0xffe9, 0xffea, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 13
  0x0000, 0xffeb, 0xffec, 0xffed, 0xffee, 0xffef, 0xfff0, 0xfff1, 0xfff2, 0xfff3, 0xfff4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 14
  0x07f9, 0xfff5, 0xfff6, 0xfff7, 0xfff8, 0xfff9, 0xfffa, 0xfffb, 0xfffc, 0xfffd, 0xfffe, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 15
};
const uint8  xJPEG_Constants::c_HuffLenDefaultChromaDC [16 ] = { 2, 2, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 0, 0, };
const uint16 xJPEG_Constants::c_HuffCodeDefaultChromaDC[16 ] = { 0x0, 0x1, 0x2, 0x6, 0xe, 0x1e, 0x3e, 0x7e, 0xfe, 0x1fe, 0x3fe, 0x7fe, 0x0, 0x0, 0x0, 0x0, };
const uint8  xJPEG_Constants::c_HuffLenDefaultChromaAC [256] =
{
   2,  2,  3,  4,  5,  5,  6,  7,  9, 10, 12,  0,  0,  0,  0,  0, 
   0,  4,  6,  8,  9, 11, 12, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  5,  8, 10, 12, 15, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  5,  8, 10, 12, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  6,  9, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  6, 10, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  7, 11, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  7, 11, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  8, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0,  9, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0, 11, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
   0, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
  10, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0,  0,  0,  0, 
};
const uint16 xJPEG_Constants::c_HuffCodeDefaultChromaAC[256] =
{//Bit=0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,     15,     
  0x0000, 0x0001, 0x0004, 0x000a, 0x0018, 0x0019, 0x0038, 0x0078, 0x01f4, 0x03f6, 0x0ff4, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 0
  0x0000, 0x000b, 0x0039, 0x00f6, 0x01f5, 0x07f6, 0x0ff5, 0xff88, 0xff89, 0xff8a, 0xff8b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 1
  0x0000, 0x001a, 0x00f7, 0x03f7, 0x0ff6, 0x7fc2, 0xff8c, 0xff8d, 0xff8e, 0xff8f, 0xff90, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 2
  0x0000, 0x001b, 0x00f8, 0x03f8, 0x0ff7, 0xff91, 0xff92, 0xff93, 0xff94, 0xff95, 0xff96, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 3
  0x0000, 0x003a, 0x01f6, 0xff97, 0xff98, 0xff99, 0xff9a, 0xff9b, 0xff9c, 0xff9d, 0xff9e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 4
  0x0000, 0x003b, 0x03f9, 0xff9f, 0xffa0, 0xffa1, 0xffa2, 0xffa3, 0xffa4, 0xffa5, 0xffa6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 5
  0x0000, 0x0079, 0x07f7, 0xffa7, 0xffa8, 0xffa9, 0xffaa, 0xffab, 0xffac, 0xffad, 0xffae, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 6
  0x0000, 0x007a, 0x07f8, 0xffaf, 0xffb0, 0xffb1, 0xffb2, 0xffb3, 0xffb4, 0xffb5, 0xffb6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 7
  0x0000, 0x00f9, 0xffb7, 0xffb8, 0xffb9, 0xffba, 0xffbb, 0xffbc, 0xffbd, 0xffbe, 0xffbf, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 8
  0x0000, 0x01f7, 0xffc0, 0xffc1, 0xffc2, 0xffc3, 0xffc4, 0xffc5, 0xffc6, 0xffc7, 0xffc8, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 9
  0x0000, 0x01f8, 0xffc9, 0xffca, 0xffcb, 0xffcc, 0xffcd, 0xffce, 0xffcf, 0xffd0, 0xffd1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 10
  0x0000, 0x01f9, 0xffd2, 0xffd3, 0xffd4, 0xffd5, 0xffd6, 0xffd7, 0xffd8, 0xffd9, 0xffda, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 11
  0x0000, 0x01fa, 0xffdb, 0xffdc, 0xffdd, 0xffde, 0xffdf, 0xffe0, 0xffe1, 0xffe2, 0xffe3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 12
  0x0000, 0x07f9, 0xffe4, 0xffe5, 0xffe6, 0xffe7, 0xffe8, 0xffe9, 0xffea, 0xffeb, 0xffec, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 13
  0x0000, 0x3fe0, 0xffed, 0xffee, 0xffef, 0xfff0, 0xfff1, 0xfff2, 0xfff3, 0xfff4, 0xfff5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 14
  0x03fa, 0x7fc3, 0xfff6, 0xfff7, 0xfff8, 0xfff9, 0xfffa, 0xfffb, 0xfffc, 0xfffd, 0xfffe, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, //RL = 15
};

//===============================================================================================================================================================================================================
// Default quantization tables generator
//===============================================================================================================================================================================================================
void xJPEG_Constants::GenerateQuantTable(uint8* QuantTable, eCmp Cmp, int32 Q, eQTLa QuantTabLayout)
{
  switch(QuantTabLayout)
  {
    case eQTLa::Default : xJPEG_Constants::GenerateQuantTableDef (QuantTable, Cmp, Q); break;
    case eQTLa::Flat    : xJPEG_Constants::GenerateQuantTableFlat(QuantTable, Cmp, Q); break;
    case eQTLa::SemiFlat: xJPEG_Constants::GenerateQuantTableSemi(QuantTable, Cmp, Q); break;
    default: assert(0); break;
  }
}
void xJPEG_Constants::GenerateQuantTableDef(uint8* QuantTable, eCmp Cmp, int32 Q)
{
  //clip to valid range - do not use 0 to avoid div-by-0
  Q = xClip(Q, 1, 100);

  //convert to percentage scaling factor
  int32 ScalingFactor = (Q < 50) ? (ScalingFactor = 5000 / Q) : (ScalingFactor = 200 - Q * 2);

  const uint8* QuantizerZ = ((Cmp == eCmp::LM) ? m_QuantTabDefLumaZ : m_QuantTabDefChromaZ);

  for(int32 i=0; i<64; i++)
  {
    //scale default table
    const int32 TempDivisor  = ((int32)(QuantizerZ[i]) * ScalingFactor + 50) / 100;
    //clip to valid range (255 for 8 bit, 32767 for 12 bits) and avoid 0 divisor 
    const uint8 QuantDivisor = (uint8)xClip(TempDivisor, 1, 255);
    QuantTable[i] = QuantDivisor;
  }
}
void xJPEG_Constants::GenerateQuantTableFlat(uint8* QuantTable, eCmp Cmp, int32 Q)
{
  uint8 Immediate[64];

  GenerateQuantTableDef(Immediate, Cmp, Q);

  int32 Sum = 0; for(int32 i = 0; i < 64; i++) { Sum += Immediate[i]; }
  int32 Avg = (Sum + 32) >> 6;

  for(int32 i=0; i<64; i++) { QuantTable[i] = (uint8)Avg; }
}
void xJPEG_Constants::GenerateQuantTableSemi(uint8* QuantTable, eCmp Cmp, int32 Q)
{
  uint8 Immediate[64];

  GenerateQuantTableDef(Immediate, Cmp, Q);

  int32 Sum = 0; for(int32 i = 0; i < 64; i++) { Sum += Immediate[i]; }
  int32 Avg = (Sum + 32) >> 6;

  for(int32 i = 0; i < 64; i++) { QuantTable[i] = (uint8)((Immediate[i] + Avg + 1)>>1); }
}
int32 xJPEG_Constants::EstimateQ(uint8* QuantTableL, uint8* QuantTableC)
{
  bool  AllOnes     = true;
  float SumPercentL = 0;
  float SumPercentC = 0;

  for(int32 i=0; i < 64; i++)
  {
    if(QuantTableL[i] != 1) { AllOnes = false; }
    float ComparePercentL = 100.0f * (float)QuantTableL[i] / (float)m_QuantTabDefLumaZ[i];
    SumPercentL += ComparePercentL;
  }
  SumPercentL /= 64.0f;

  for(int32 i=0; i < 64; i++)
  {
    if(QuantTableC[i] != 1) { AllOnes = false; }
    float ComparePercentC = 100.0f * (float)QuantTableC[i] / (float)m_QuantTabDefChromaZ[i];
    SumPercentC += ComparePercentC;
  }
  SumPercentC /= 64.0f;

  float EstimatedQ[2];
  if(AllOnes)
  {
    EstimatedQ[0] = EstimatedQ[1] = 100.0f;
  }
  else
  {
    if(SumPercentL <= 100.0) { EstimatedQ[0] = (200.0f - SumPercentL) / 2.0f; }
    else                     { EstimatedQ[0] = 5000.0f / SumPercentL; }
    if(SumPercentC <= 100.0) { EstimatedQ[1] = (200.0f - SumPercentC) / 2.0f; }
    else                     { EstimatedQ[1] = 5000.0f / SumPercentC; }
  }
  int32 IntEstimatedQ[2] = { (int32)std::round(EstimatedQ[0]), (int32)std::round(EstimatedQ[1]) };
  return (IntEstimatedQ[0] == IntEstimatedQ[1]) ? IntEstimatedQ[0] : NOT_VALID;
}

//===============================================================================================================================================================================================================

} //end of namespace PMBB_NAMESPACE::JPEG
