# JOptEnc

JPEG encoder with Rate Distortion Optimized Quantized Quantization (RDOQ)

----  

## 1. Description

The software is a JPEG encoder that is implemented in a logical, structured way for simple modification for research and education purposes. It provides means to encode JPEG images with better objective quality (PSNR) or better compression ratio than a regular JPEG encoder. The produced bitstream is compatible with any JPEG decoding software. The optimization is performed at the quantized AC transform coefficients level. The AC quantized transform coefficients values are modified using a cost function with Lagrange multiplier [2] where the Lagrange multiplier value is calculated dynamically for the current Q value specified for compression.

## 2. Authors

* Jakub Stankowski       - Poznan University of Technology, Poznań, Poland  
* Krzysztof Klimaszewski - Poznan University of Technology, Poznań, Poland
* Tomasz Grajek          - Poznan University of Technology, Poznań, Poland
* Adam Grzelka           - Poznan University of Technology, Poznań, Poland

## 3. License

### 3.1. TLDR

3-Clause BSD License

### 3.2. Full text

```txt
The copyright in this software is being made available under the BSD
License, included below. This software may be subject to other third party
and contributor rights, including patent rights, and no such rights are
granted under this license.

Copyright (c) 2020-2024, Jakub Stankowski, All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
 * Neither the name of the ISO/IEC nor the names of its contributors may
   be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
```

## 4. Building 

Building the JOptEnc software requires using CMake (https://cmake.org/) and C++17 conformant compiler (e.g., GCC >= 10.0, clang >= 13.0, MSVC >= 19.15). 

The JOptEnc application and its build system are designed to create the fastest possible binary. On x86-64 microarchitectures, the build system can create four versions of compiled application, each optimized for one predefined x86-64 Microarchitecture Feature Levels [x86-64, x86-64-v2, x86-64-v3, x86-64-v4] (defined in https://gitlab.com/x86-psABIs/x86-64-ABI). The final binary consists of these four optimized variants and a runtime dynamic dispatcher. The dispatcher uses CPUID instruction to detect available instruction set extensions and selects the fastest possible code path. 

The JOptEnc CMake project defines the following parameters:

| Variable | Type | Description |
| :------- | :--- | :---------- |
| `PMBB_GENERATE_MULTI_MICROARCH_LEVEL_BINARIES`        | BOOL | Enables generation of multiple code paths, optimized for each variant of x86-64 Microarchitecture Feature Levels. |
| `PMBB_GENERATE_SINGLE_APP_WITH_WITH_RUNTIME_DISPATCH` | BOOL | Enables building single application with runtime dynamic dispatch. Requires `PMBB_GENERATE_MULTI_MICROARCH_LEVEL_BINARIES=True`. |
| `PMBB_GENERATE_DEDICATED_APPS_FOR_EVERY_MFL`          | BOOL | Enables building multiple applications, each optimized for selected x86-64 Microarchitecture Feature Level. Requires `PMBB_GENERATE_MULTI_MICROARCH_LEVEL_BINARIES=True`. |
| `PMBB_BUILD_WITH_MARCH_NATIVE`                        | BOOL | Enable option to force compiler to tune generated code for the micro-architecture and ISA extensions of the host CPU. This option works only when GCC or clang compiler is used since MSVC is unable to adapt native architecture. Conflicts with `PMBB_GENERATE_MULTI_MICROARCH_LEVEL_BINARIES`. Generated binary is not portable across different microarchitectures. |
| `PMBB_GENERATE_TESTING`                               | BOOL | Enables generation of unit tests for critical data processing routines. |

For users convenience, we prepared a set of scripts for easy "one click" configure and build:
* *configure_and_build.bat* - for Windows users
* *configure_and_build.sh* - for Unix/Linux users

For developers convenience, we prepared a set of scripts for easy "one click" configure in development mode - without multi-architecture build and dynamic dispatch:
* *configure_for_developement.bat* - for Windows users
* *configure_for_developement.sh* - for Unix/Linux users

The *configure_and_build* and *configure_for_developement* uses different set of CMake parameters:

| Variable | configure_and_build | configure_for_developement |
| :------- | :------------------ | :------------------------- |
| `PMBB_GENERATE_MULTI_MICROARCH_LEVEL_BINARIES`        | True  | False |
| `PMBB_GENERATE_SINGLE_APP_WITH_WITH_RUNTIME_DISPATCH` | True  | False |
| `PMBB_GENERATE_DEDICATED_APPS_FOR_EVERY_MFL`          | False | False |
| `PMBB_BUILD_WITH_MARCH_NATIVE`                        | False | True  |
| `PMBB_GENERATE_TESTING`                               | False | True  |

## 5. Usage

### 5.1. Commandline parameters

Commandline parameters are parsed from left to right. Multiple config files are allowed. Moreover, config file parameters can be overridden or added via commandline.  

#### General parameters

| Cmd | ParamName       | Description |
|:----|:----------------|:------------|
|-i   |  InputFile      | Input RAW/PNG/BMP file path |
|-o   |  OutputFile     | Output JPEG/MJPEG file path [optional] |
|-r   |  ReconFile      | Reconstructed RAW/PNG/BMP file path [optional] |
|-ff  |  FileFormat     | Format of input sequence (optional, default=RAW) [RAW, PNG, BMP] |
|-ps  |  PictureSize    | Size of input sequences (WxH) |
|-pw  |  PictureWidth   | Width of input sequences  |
|-ph  |  PictureHeight  | Height of input sequences |
|-pf  |  PictureFormat  | Picture format as defined by FFMPEG pix_fmt i.e. yuv420p10le |
|-pt  |  PictureType    | YCbCr / RGB   (optional, relevant to RAW format, defalult YCbCr) |
|-bd  |  BitDepth       | Bit depth     (optional, relevant to RAW format, default 8)  |
|-cf  |  ChromaFormat   | Chroma format (optional, default 420) [420, 422, 444] |
|-sf  |  StartFrame     | Start frame   (optional, default 0)  |
|-nf  |  NumberOfFrames | Number of frames to be processed (optional, default -1=all) |
|-fr  |  FrameRate      | Sequence framerate - used to calculate bitrate (default 1) [optional] |

PictureSize parameter can be used interchangeably with PictureWidth and PictureHeight. If PictureSize parameter is present, the PictureWidth and PictureHeight arguments are ignored.
PictureFormat parameter can be used interchangeably with PictureType, BitDepth and ChromaFormat. If PictureFormat parameter is present, the PictureType, BitDepth and ChromaFormat arguments are ignored.

PictureFormat, PictureType, BitDepth and ChromaFormat are not necessary for PNG/BMP input format.

#### JPEG specific parameters

| Cmd | ParamName        | Description |
|:----|:-----------------|:------------|
|-q   |  Quality         | JPEG Quality (Q, 0-100, 0=lowest quality, 100=highest quality) |
|-ri  |  RestartInterval | Restart interval in number of MCUs [0=disabled] (optional, default 0) |

#### JPEG specific parameters

| Cmd | ParamName         | Description |
|:----|:------------------|:------------|
|-rol | OptimizeLuma      | Apply RDOQ to luma blocks   (default 1) [optional] |
|-roc | OptimizeChroma    | Apply RDOQ to chroma blocks (default 1) [optional] |
|-rpz | ProcessZeroCoeffs | Try to optimize coeffs initially quantized to 0 (default 0) [optional] |
|-rnp | NumBlockOptPasses | Number of optimization passes over one block (default 1) [optional] |
|-qtl | QuantTabLayout    | Select quantization table layout used during encoding: [0 = default (RFC2435), 1 = flat, 2 = semi-flat] (default 0) [optional] |

#### Validation parameters

| Cmd | ParamName        | Description |
|:----|:-----------------|:------------|
|-ipa | InvalidPelActn   | Select action taken if invalid pixel value is detected (optional, default=STOP) [SKIP = disable pixel value checking, WARN = print warning and ignore, STOP = stop execution, CNCL = try to conceal by clipping to bit depth range] |
|-nma | NameMismatchActn | Select action taken if parameters derived from filename are different than provided as input parameters. Checks resolution, bitdepth and chroma format. (optional, default=WARN) [SKIP = disable checking, WARN = print warning and ignore, STOP = stop execution] |

#### Application parameters

| Cmd | ParamName        | Description |
|:----|:-----------------|:------------|
|-cp  | CalkPSNR         | Calculate PSNR for reconstructed picture (default 1) [optional] |
|-v   | VerboseLevel     | Verbose level (optional, default=1) |

#### External config file

| Cmd | ParamName        | Description |
|:----|:-----------------|:------------|
| -c  | n/a              | Valid path to external config file - in INI format [optional] |

Multiple config files can be provided by using multiple "-c" arguments. Config files are processed in arguments order. Content of config files are merged while repeating values are overwritten.

#### Dynamic dispatcher parameters

| Cmd                | Description |
|:-------------------|:------------|
| --DispatchForceMFL | Force dispatcher to selected microarchitecture (optional, default=UNDEFINED) [x86-64, x86-64-v2, x86-64-v3, x86-64-v4]. Forcing a selection of microarchitecture level not supported by CPU will lead to "illegal instruction" exception.
| --DispatchVerbose  | Verbose level for runtime dispatch module (optional, default=0) |

### 5.2. Verbose level TODO

| Value | Behavior |
|:------|:---------|
| 0 | final (average) metric values only |
| 1 | 0 + configuration + TODO |
| 2 | 1 + argc/argv + TODO |
| 3 | 2 + TODO |
| 4 | 3 + RDOQ debug data (lambda) |

### 5.3. Compile-time parameters

| Parameter name | Default value | Description |
|:---------------|:--------------|:------------|
| USE_SIMD       | 1             | use SIMD (to be precise... use SSE 4.1 or AVX2 or AVX512) |

### 5.4. Examples

#### 5.4.1. Commandline parameters example

Encode planar RAW YCbCr 4:2:0 file:  
`JOptEnc -i "A.yuv" -pw 1920 -ph 1080 -o "A.mjpeg" -r "A.yuv" -q 80 -v 3`  

Encode PNG file:  
`JOptEnc -i "A.png" -ps 512x384 -o "A.jpeg" -r "I01_rec.png" -ff PNG -q 90 -v 3`  

Encode BMP file:  
`JOptEnc -i "A.bmp" -ps 512x384 -o "A.jpeg" -r "I01_rec.bmp" -ff BMP -q 90 -v 3`  

#### 5.4.2. Config file examples

```ini
InputFile    = "A.yuv"
OutputFile   = "A.mjpeg"
ReconFile    = "A.yuv"
PictureSize  = "1920x1080"
Quality      = 80
VerboseLevel = 1
```

#### 5.4.3. Extended usage example

Extended usage example can be found in the [example folder](./example/JOptEnc/)

### 5.5. Pixel values checking notes (InvalidPelActn)

Before encoding, the software scans the content of the RAW file (YUV, RGB, BGR, GRB, etc.) in order to evaluate if all pixel values are in range `[0, MaxVal]` where `MaxVal = (1<<BitDepth) - 1`. If invalid pel is detected, the software can take the following actions based on InvalidPelActn parameter value:
* SKIP - disable pixel value checking
* WARN - print warning and ignore invalid picture element (pel) values (may lead to unreliable metrics value)
* STOP - print warning and stop execution
* CNCL - print warning and try to conceal the picture element (pel) value by clipping to the highest value within the bit depth range

### 5.6. Filename (filepath) parameters mismatch checking notes (InvalidPelAction)

Before opening the RAW file (YUV, RGB, BGR, GRB, etc.) the software tries to derive important video parameters (resolution, a bit depth and chroma format) from the file name and file path. If a mismatch between parameters provided from commandline (or config file) and derived values is detected, the software can take following actions based on NameMismatchActn parameter value:
* SKIP - disable video parameters checking and use values declared in configuration
* WARN - print warning and use values declared in configuration, discarding those from file name
* STOP - print warning and stop execution

### 5.7. PNG/BMP mode

The JOptEnc allows to encoding not only for single image or video files, but for lists of indexed images in the PNG/BMP format. 

The name of the input file should contain a format string as defined in C++20 std::format [ISO/IEC 14882:2020] in the form of `{:d}` with optional modifiers. The file name is determined by formatting the input string using image index.

The software expects lists of consequently numbered files. The first file index can be equal to 0 or 1. The last file is detected by checking the existence of consecutive files. The first missing index is treated as the end of the file list.

Examples:
* The list of files {img001.png, img002.png, img003.png} should be specified as `img{:03d}.png`. 
* The list of files {img000.png, img001.png, img002.png, img004.png}, specified as `img{:03d}.png`, will be processed for 0,1, and 2 indexes only. The `img002.png` will be detected as the last image in the list.

## 6. File structure

| Subfolder  | Description |
|:-----------|:------------|
| apps       | application sources |
| cmake      | files related to cmake build system |
| example    | usage examples |
| libs       | library sources |
| thirdparty | third party sources that had to be modified to accommodate it into JOptEnc |

## R. References

* [1] T. Grajek, J. Stankowski, “Rate-Distortion Optimized Quantization in Motion JPEG”, 30th International Conference in Central Europe on Computer Graphics, Visualization and Computer Vision : WSCG 2022, 2022, ISSN: 2464–4617
* [2] Karczewicz M., Ye Y., Chong I., “Rate Distortion Optimized Quantization”, document ITU-T SG16 Q.6, VCEG-AH21, 2008