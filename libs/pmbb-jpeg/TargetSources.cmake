set(SRCLIST_COMMON_H src/xCommonDefJPEG.h)
set(SRCLIST_COMMON_C src/xCommonJPEG.cpp)

set(SRCLIST_CONST_H src/xJPEG_Constants.h  )
set(SRCLIST_CONST_C src/xJPEG_Constants.cpp)

set(SRCLIST_BLOCKS_H src/xJPEG_Entropy.h   src/xJPEG_Huffman.h   src/xJPEG_HuffmanDefault.h   src/xJPEG_Quant.h   src/xJPEG_Scan.h   src/xJPEG_Transform.h   src/xJPEG_TransformConstants.h  )
set(SRCLIST_BLOCKS_C src/xJPEG_Entropy.cpp src/xJPEG_Huffman.cpp src/xJPEG_HuffmanDefault.cpp src/xJPEG_Quant.cpp src/xJPEG_Scan.cpp src/xJPEG_Transform.cpp src/xJPEG_TransformConstants.cpp)

set(SRCLIST_CONTAINER_H src/xJFIF.h  )
set(SRCLIST_CONTAINER_C src/xJFIF.cpp)

set(SRCLIST_CODEC_H src/xJPEG_CodecCommon.h   src/xJPEG_CodecSimple.h   src/xJPEG_Encoder.h  )
set(SRCLIST_CODEC_C src/xJPEG_CodecCommon.cpp src/xJPEG_CodecSimple.cpp src/xJPEG_Encoder.cpp)

set(SRCLIST_PUBLIC  ${SRCLIST_COMMON_H} ${SRCLIST_CONST_H} ${SRCLIST_BLOCKS_H} ${SRCLIST_CONTAINER_H} ${SRCLIST_CODEC_H})
set(SRCLIST_PRIVATE ${SRCLIST_COMMON_C} ${SRCLIST_CONST_C} ${SRCLIST_BLOCKS_C} ${SRCLIST_CONTAINER_C} ${SRCLIST_CODEC_C})

target_sources(${PROJECT_NAME} PRIVATE ${SRCLIST_PRIVATE} PUBLIC ${SRCLIST_PUBLIC})
source_group(Common      FILES ${SRCLIST_COMMON_H} ${SRCLIST_COMMON_C})
source_group(Constants   FILES ${SRCLIST_CONST_H} ${SRCLIST_CONST_C})
source_group(JPEG Blocks FILES ${SRCLIST_BLOCKS_H} ${SRCLIST_BLOCKS_C})
source_group(Containers  FILES ${SRCLIST_CONTAINER_H} ${SRCLIST_CONTAINER_C})
source_group(Codecs      FILES ${SRCLIST_CODEC_H} ${SRCLIST_CODEC_C})












