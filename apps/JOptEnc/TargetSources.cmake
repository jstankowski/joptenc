set(SRCLIST_APP src/main_JOptEnc.cpp src/xAppJOptEnc.h src/xAppJOptEnc.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${SRCLIST_APP})
source_group(App FILES ${SRCLIST_APP})

