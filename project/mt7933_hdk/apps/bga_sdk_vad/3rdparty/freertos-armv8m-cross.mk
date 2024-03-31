
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(tool ${SDK_PATH}/tools/gcc/linux/gcc-arm-none-eabi)

set(CMAKE_C_COMPILER ${tool}/bin/arm-none-eabi-gcc)
set(CMAKE_C_FLAGS --specs=nosys.specs)

set(CMAKE_CXX_COMPILER ${tool}/bin/arm-none-eabi-g++)
set(CMAKE_CXX_FLAGS --specs=nosys.specs)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

