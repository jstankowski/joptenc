set(SRCLIST_COMMON_H src/xCommonDefCMPR.h src/xMiscUtilsCMPR.h  )
set(SRCLIST_COMMON_C                      src/xMiscUtilsCMPR.cpp)

set(SRCLIST_BB_H src/xBitstream.h   src/xByteBuffer.h  )
set(SRCLIST_BB_C src/xBitstream.cpp src/xByteBuffer.cpp)

set(SRCLIST_PUBLIC  ${SRCLIST_COMMON_H} ${SRCLIST_BB_H} )
set(SRCLIST_PRIVATE ${SRCLIST_COMMON_C} ${SRCLIST_BB_C} )

target_sources(${PROJECT_NAME} PRIVATE ${SRCLIST_PRIVATE} PUBLIC ${SRCLIST_PUBLIC})
source_group(Common  FILES ${SRCLIST_COMMON_H} ${SRCLIST_COMMON_C})
source_group(ByteBit FILES ${SRCLIST_BB_H} ${SRCLIST_BB_C})









