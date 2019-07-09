cmake_minimum_required(VERSION 3.14)
project(parser)

set(CMAKE_CXX_FLAGS "-O3 -Wextra -Wall -Wpedantic -Wconversion -Wsign-conversion")

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE HDRS ${parser_SOURCE_DIR}/src/*.h  )
file(GLOB_RECURSE SRCS ${parser_SOURCE_DIR}/src/*.cpp)

add_executable(parser ${HDRS} ${SRCS})