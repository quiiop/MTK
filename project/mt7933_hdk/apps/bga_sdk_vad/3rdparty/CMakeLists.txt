cmake_minimum_required(VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${PROJECT_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROJECT_CFLAGS}")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -I${CMAKE_CURRENT_SOURCE_DIR}/porting/include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/porting/include")

set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wno-unused-variable")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-variable")

##############################################################################

add_subdirectory(opus)

add_subdirectory(portaudio)
