
set(OPUS_ENABLE_FLOAT_API OFF CACHE BOOL "disable floating point API")
set(OPUS_FIXED_POINT ON CACHE BOOL "use fixed point")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -O3 -ffast-math")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math")

add_subdirectory(opus)

