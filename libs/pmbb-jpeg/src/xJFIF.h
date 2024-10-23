/*
    SPDX-FileCopyrightText: 2020-2024 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_Constants.h"
#include "xByteBuffer.h"
#include <numeric>
#include <vector>
#include <array>

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================

class xJFIF
{
public:
  using tStr   = std::string;
  using tByteV = std::vector<byte>;
  static constexpr uint32 c_JFIF = xMakeFourCC('J', 'F', 'I', 'F');

  enum class eMarker
  {
    SOI   = 0xD8, //Start Of Image 

    APP0  = 0xE0, //JFIF APP0 segment marker, 
    APP15 = 0xEF, 

    DRI   = 0xDD, //Define Restart Interval

    SOF0  = 0xC0, //Start Of Frame (baseline JPEG)
    SOF1  = 0xC1,
    SOF2  = 0xC2,
    SOF3  = 0xC3,
    SOF5  = 0xC5,
    SOF6  = 0xC6,
    SOF7  = 0xC7,
    JPG   = 0xC8,
    SOF9  = 0xC9,
    SOF10 = 0xCA,
    SOF11 = 0xCB,
    SOF13 = 0xCD,
    SOF14 = 0xCE,
    SOF15 = 0xCF,

    DHT   = 0xC4, //Define Huffman Table 
    DAC   = 0xCC, //Define Arithmetic Table
    DQT   = 0xDB, //Define Quantization Table
     
    SOS   = 0xDA, //Start Of Scan 
     
    RST0  = 0xD0, //Restart(s)
    RST1  = 0xD1,
    RST2  = 0xD2,
    RST3  = 0xD3,
    RST4  = 0xD4,
    RST5  = 0xD5,
    RST6  = 0xD6,
    RST7  = 0xD7,

    EOI   = 0xD9, //End Of Image

    DNL   = 0xDC,
    DHP   = 0xDE,
    EXP   = 0xDF,
    JPG0  = 0xF0,
    JPG13 = 0xFD,
    COM   = 0xFE,
    TEM   = 0x01,

    ERR   = 0x00,
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  class xFileParser
  {
  protected:
    static const int32 m_BufferSize = 4194304; //4MB = 1024 pages

  protected:
    xStream*    m_File;
    int64       m_BufferFilePos;
    xByteBuffer m_Buffer;

  public:
    void create(xStream* File)
    {
      m_File          = File;
      m_BufferFilePos = NOT_VALID;
      m_Buffer.create(m_BufferSize);
    }
    void destroy()
    {
      m_Buffer.destroy();
      m_File = nullptr;
    }
    int64 SeekNextSegment(eMarker Marker)
    {
      int64 CurrFilePosiotion = m_File->tellR();
      if(m_BufferFilePos != CurrFilePosiotion) //reset
      {
        m_Buffer.reset();
        m_BufferFilePos = CurrFilePosiotion;
        int64 Readed = m_Buffer.read(m_File);
        if(Readed == 0) { return NOT_VALID; }
      }
      while(true)
      {        
        int64 PositionInBuffer = FindSegment(&m_Buffer, Marker);

        if(PositionInBuffer == NOT_VALID)
        {
          //try to load more
          if(m_File->end()) { return NOT_VALID; }
          m_File->seekR(-1, xStream::eSeek::Cur);
          CurrFilePosiotion--;
          m_BufferFilePos = CurrFilePosiotion;
          int64 Readed = m_Buffer.read(m_File);
          if(Readed == 0) { return NOT_VALID; }
        }
        else
        {
          int64 PositionInFile = m_BufferFilePos + PositionInBuffer;
          m_File->seekR(PositionInFile, xStream::eSeek::Beg);
          m_Buffer.modifyRead((int32)PositionInBuffer);
          m_BufferFilePos = PositionInFile;
          return PositionInFile;
        }
      }
      return NOT_VALID;
    }    
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  class xAPP0
  {
  protected:
    int32  m_VersionMajor   = 0;
    int32  m_VersionMinor   = 0;
    int32  m_DensityUnits   = 0; // 0 = no units, x/y-density specify the aspect ratio instead; 1 = x/y-density are dots/inch; 2 = x/y-density are dots/cm
    int32  m_DensityX       = 0;
    int32  m_DensityY       = 0;
    int32  m_ThumbnailSizeX = 0;
    int32  m_ThumbnailSizeY = 0;
    tByteV m_ThumbnailData;

  public:
    int32 Absorb(xByteBuffer* Input , int32 SegmentLength);
    int32 Emit  (xByteBuffer* Output); 
    void  InitDefault();
    bool  Validate   ();
    int32 getLength  () { return 16 + m_ThumbnailSizeX * m_ThumbnailSizeY * 3; }
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  
  class xQuantTable
  {
  protected:
    int32  m_Idx       = NOT_VALID;
    int32  m_Precision = NOT_VALID; //008bit, 1=16bit
    tByteV m_Table;

  public:
    xQuantTable() : m_Table(64) { }
    int32 Absorb   (xByteBuffer* Input );
    int32 Emit     (xByteBuffer* Output) const; 
    void  Init     (uint8 Idx, eCmp Cmp, int32 Quality, eQTLa QuantTabLayout = eQTLa::Default);
    bool  Validate () const;
    tStr  Format   (const tStr& Prefix = "  ") const;

    int32  getLength   () const { return 1 + (m_Precision == 1 ? 128 : 64); }
    int32  getIdx      (         ) const { return m_Idx; }
    void   setIdx      (int32 Idx)       {m_Idx = (int8)Idx; }
    int32  getPrecision(               ) const { return m_Precision; }
    void   setPrecision(int32 Precision)       { m_Precision = (int8)Precision; }
    
    const tByteV& getTableData(                 ) const { return m_Table; }
    void          setTableData(const tByteV& QTD)       { m_Table = QTD; }
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  class xSOF0
  {
  protected:
    int32  m_Width         = NOT_VALID;
    int32  m_Height        = NOT_VALID;
    int32  m_BitDepth      = NOT_VALID;
    int32  m_NumComponents = NOT_VALID;
    eCmp   m_CmpId         [xJPEG_Constants::c_MaxComponents];
    int32  m_SamplingFactor[xJPEG_Constants::c_MaxComponents];
    int32  m_QuantTableId  [xJPEG_Constants::c_MaxComponents];

  public:
    int32  Absorb   (xByteBuffer* Input );
    int32  Emit     (xByteBuffer* Output) const; 
    void   Init     (int32 Height, int32 Width, int32 BitDepth, eCrF ChromaFormat, int32 NumQuantTables);
    bool   Validate () const;
    int32  getLength() const { return 8 + m_NumComponents * 3; }


    eCrF   DetermineChromaFormat() const;
    int8   GetSamplingFactorH(int32 Idx) const { return ((m_SamplingFactor[Idx] & 0xF0) >> 4); }
    int8   GetSamplingFactorV(int32 Idx) const { return ( m_SamplingFactor[Idx] & 0x0F);       }
           
  public:  
    void  setWidth         (int32 Width        )       { m_Width = Width; }
    int32 getWidth         (                   ) const { return m_Width; }
    void  setHeight        (int32 Height       )       { m_Height = Height; }
    int32 getHeight        (                   ) const { return m_Height; }
    void  setBitDepth      (int32 BitDepth     )       { m_BitDepth = BitDepth; }
    int32 getBitDepth      (                   ) const { return m_BitDepth; }
    void  setNumComponents (int32 NumComponents)       { m_NumComponents = NumComponents; }
    int32 getNumComponents (                   ) const { return m_NumComponents; }
    void  setCmpId         (eCmp CmpId,           eCmp  Cmp)       { m_CmpId[(int32)Cmp] = CmpId; }
    eCmp  getCmpId         (                      eCmp  Cmp) const { return m_CmpId[(int32)Cmp]; }
    void  setSamplingFactor(int32 SamplingFactor, eCmp  Cmp)       { m_SamplingFactor[(int32)Cmp] = SamplingFactor; }
    int32 getSamplingFactor(                      eCmp  Cmp) const { return m_SamplingFactor[(int32)Cmp]; }
    void  setQuantTableId  (int32 QuantTableId,   eCmp  Cmp)       { m_QuantTableId[(int32)Cmp] = QuantTableId; }
    int32 getQuantTableId  (                      eCmp  Cmp) const { return m_QuantTableId[(int32)Cmp]; }
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  class xHuffTable
  {
  public:
    using tCodeL = std::array<byte, 16>;

    enum class eHuffClass : int8
    {
      Invalid = NOT_VALID,
      DC = 0,
      AC = 1,
    };

  protected:
    uint8      m_Idx   = 0;
    eHuffClass m_Class = eHuffClass::Invalid;
    tCodeL     m_CodeLengths = { 0 };
    tByteV     m_CodeSymbols;

  public:
    int32 Absorb     (xByteBuffer* Input );
    int32 Emit       (xByteBuffer* Output) const; 
    void  InitDefault(uint8 Idx, eHuffClass Class, eCmp Cmp);
    bool  Validate   () const;
    int32 getLength  () const { return 1 + 16 + (int32)m_CodeSymbols.size(); }

  public:
    int32      getIdx  (         ) const { return m_Idx;   }
    void       setIdx  (int32 Idx)       { m_Idx = (uint8)Idx;   }
    eHuffClass getClass(             ) const { return m_Class; }
    void       setClass(eHuffClass HC)       { m_Class = HC; }
    
    const tCodeL& getCodeLengths() const { return m_CodeLengths; }
    const tByteV& getCodeSymbols() const { return m_CodeSymbols; }

    bool isDC() const { return (m_Class == eHuffClass::DC);}
    bool isAC() const { return (m_Class == eHuffClass::AC);}
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

  class xSOS
  {
  protected:
    int32  m_NumComponents;
    eCmp   m_CmpId      [xJPEG_Constants::c_MaxComponents];
    int32  m_HuffTableId[xJPEG_Constants::c_MaxComponents];
    int32  m_SpectralSelectionStart;
    int32  m_SpectralSelectionEnd;
    int32  m_SuccessiveApproximation;

  public:
    int32  Absorb(xByteBuffer* Input );
    int32  Emit  (xByteBuffer* Output)  const;
    void   Init     (int32 NumComponents, int32 LumaHuffTabIdx, int32 ChromaHuffTabIdx);
    bool   Validate () const;
    int32  getLength() const { return 6 + m_NumComponents * 2; }

  public:
    void   setNumComponents(int32 NumComponents)       { m_NumComponents = NumComponents; }
    int32  getNumComponents(                   ) const { return m_NumComponents; }
    void   setCmpId        (eCmp CmpId, eCmp  Cmp)       { m_CmpId[(int32)Cmp] = CmpId; }
    eCmp   getCmpId        (            eCmp  Cmp) const { return m_CmpId[(int32)Cmp]; }
    void   setHuffTableId  (int32 HuffTableIdDC, int32 HuffTableIdAC, eCmp Cmp)       { m_HuffTableId[(int32)Cmp] = (((HuffTableIdDC & 0x03) << 4) + (HuffTableIdAC & 0x03)); }
    int32  getHuffTableIdDC(                                          eCmp Cmp) const { return ((m_HuffTableId[(int32)Cmp] >> 4) & 0x03); }
    int32  getHuffTableIdAC(                                          eCmp Cmp) const { return ((m_HuffTableId[(int32)Cmp]     ) & 0x03); }
  };

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  
public:
  static bool    ReadSOI         (xByteBuffer* Input ) { return (xReadMarker(Input )==eMarker::SOI); }
  static void    WriteSOI        (xByteBuffer* Output) { xWriteMarker(Output, eMarker::SOI); }

  static bool    ReadAPP0        (xByteBuffer* Input , xAPP0& APP0);
  static void    WriteAPP0       (xByteBuffer* Output, xAPP0& APP0);
  static void    WriteDefaultAPP0(xByteBuffer* Output);

  static bool    ReadDQT         (xByteBuffer* Input ,       std::vector<xQuantTable>& QuantTables);
  static void    WriteDQT        (xByteBuffer* Output, const std::vector<xQuantTable>& QuantTables);

  static bool    ReadDRI         (xByteBuffer* Input , int32& RestartInterval);
  static void    WriteDRI        (xByteBuffer* Output, int32  RestartInterval);

  static bool    ReadSOF0        (xByteBuffer* Input , xSOF0& SOF0);
  static void    WriteSOF0       (xByteBuffer* Output, xSOF0& SOF0);
  static void    WriteSOF0       (xByteBuffer* Output, int32 Height, int32 Width, int32 BitDepth, eCrF ChromaFormat, int32 NumQuantTables);
  
  static bool    ReadDHT         (xByteBuffer* Input , std::vector<xHuffTable>& HuffTables);
  static void    WriteDHT        (xByteBuffer* Output, std::vector<xHuffTable>& HuffTables);
  static void    WriteDefaultDHT (xByteBuffer* Output);

  static bool    ReadSOS         (xByteBuffer* Input , xSOS& SOS);
  static bool    SkipSOS         (xByteBuffer* Input            );
  static void    WriteSOS        (xByteBuffer* Output, xSOS& SOS);
  static void    WriteSOS        (xByteBuffer* Output, int32 NumComponents, int32 LumaHuffTabIdx, int32 ChromaHuffTabIdx);
    
  static bool    ReadData        (byte* Payload, int32 Size, xByteBuffer* Input ) { return xReadMemmory (Payload, Size, Input ); }
  static void    WriteData       (xByteBuffer* Output, xByteBuffer* Payload     ) { xWriteBuffer (Output, Payload      ); }
  static void    WriteData       (xByteBuffer* Output, byte* Payload, int32 Size) { xWriteMemmory(Output, Payload, Size); }

  static int8    ReadRST         (xByteBuffer* Input);
  static void    WriteRST        (xByteBuffer* Output, uint8 RestartIdx);
  
  static bool    ReadEOI         (xByteBuffer* Input ) { return (xReadMarker(Input )==eMarker::EOI); }
  static void    WriteEOI        (xByteBuffer* Output) { xWriteMarker(Output, eMarker::EOI); }

public:
  static eMarker IdentifySegment (xByteBuffer* Input ) { return xPeekMarker(Input ); }
  static int32   FindSegment     (xByteBuffer* Input , eMarker Marker);
  static int64   SeekNextSegment (std::ifstream* Input, eMarker Marker);

public: 
  static void    AddStuffing     (xByteBuffer* Output, xByteBuffer* Input);
  static void    RemoveStuffing  (xByteBuffer* Output, xByteBuffer* Input);
      
protected:
  static uint8   xPeek8          (xByteBuffer* Input ) { return Input ->peekU8    ();  }
  static uint16  xPeek16         (xByteBuffer* Input ) { return Input ->peekU16_BE(); }
  static eMarker xPeekMarker     (xByteBuffer* Input ) { uint16 D = xPeek16(Input ); if((D>>8)==0xFF) { uint8 M = D & 0x00FF; if(M >= 0xC0) { return eMarker(M); } } return eMarker::ERR; }
                                
  static uint8   xRead8          (xByteBuffer* Input ) { return Input ->extractU8    ();  }
  static uint16  xRead16         (xByteBuffer* Input ) { return Input ->extractU16_BE(); }
  static uint32  xReadFourCC     (xByteBuffer* Input ) { return Input ->extractU32_LE(); }
  static bool    xReadMemmory    (byte* DstPtr, int32 Size,  xByteBuffer* Input) { return Input ->extractBytes(DstPtr, Size); }
  static bool    xReadVector     (tByteV& DstVec, xByteBuffer* Input) { return Input ->extractBytes(DstVec.data(), (int32)DstVec.size()); }
  static eMarker xReadMarker     (xByteBuffer* Input ) { uint8 FF = xRead8(Input ); if(FF==0xFF) { uint8 M = xRead8(Input ); if(M >= 0xC0) { return eMarker(M); } } return eMarker::ERR; }

  static void    xSkip           (xByteBuffer* Input , int32 Size) { Input ->modifyRead(Size); }

  static void    xDispose8       (xByteBuffer* Output, uint8  Val) { Output->disposeU8    (Val); }
  static void    xDispose16      (xByteBuffer* Output, uint16 Val) { Output->disposeU16_BE(Val); }
  static void    xDisposeMarker  (xByteBuffer* Output, eMarker Marker) { xDispose16(Output, (uint16)0xFF00 | (uint16)Marker); }
                         
  static void    xWrite8         (xByteBuffer* Output, uint8  Val) { Output->appendU8    (Val); }
  static void    xWrite16        (xByteBuffer* Output, uint16 Val) { Output->appendU16_BE(Val); }
  static void    xWriteFourCC    (xByteBuffer* Output, uint32 Val) { Output->appendU32_LE(Val); }
  static void    xWriteMemmory   (xByteBuffer* Output, const byte* SrcPtr, int32 Size) { Output->appendBytes(SrcPtr, Size); }                
  static void    xWriteBuffer    (xByteBuffer* Output, xByteBuffer* Input ) { Output->append(Input ); }
  static void    xWriteVector    (xByteBuffer* Output, const tByteV& SrcVec) { Output->appendBytes(SrcVec.data(), (int32)SrcVec.size()); }
  static void    xWriteMarker    (xByteBuffer* Output, eMarker Marker) { xWrite8(Output, 0xFF); xWrite8(Output, (uint8)Marker); }

  static bool xFitsU8 (int32 V) { return V >= (int32)uint8_min  && V <= (int32)uint8_max ; }
  static bool xFitsU16(int32 V) { return V >= (int32)uint16_min && V <= (int32)uint16_max; }
};

//=============================================================================================================================================================================

} //end of namespace PMBB_NAMESPACE::JPEG

