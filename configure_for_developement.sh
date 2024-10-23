cmake --build buildLD --target clean
cmake -S ./ -B buildLD -DPMBB_GENERATE_TESTING:BOOL=True -DPMBB_BUILD_WITH_MARCH_NATIVE:BOOL=True -DPMBB_GENERATE_MULTI_MICROARCH_LEVEL_BINARIES:BOOL=False
cmake --build buildLD --clean-first --config Release --parallel $(nproc)

