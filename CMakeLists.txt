cmake_minimum_required(VERSION 3.17)
project(TurboTCP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 兼容我们机器上神秘的老版本gcc
find_package(Threads REQUIRED)

include_directories(src)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.hpp")

add_executable(turbotcp ${SOURCES})
target_link_libraries(turbotcp Threads::Threads)