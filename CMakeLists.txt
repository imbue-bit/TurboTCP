cmake_minimum_required(VERSION 3.10)
project(TurboTCP)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 显式包含 include 目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# 兼容我们机器上神秘的老版本gcc
find_package(Threads REQUIRED)
file(GLOB SOURCES "src/*.cpp")
add_executable(tcp-turbo ${SOURCES})
target_link_libraries(tcp-turbo PRIVATE Threads::Threads)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
target_compile_options(tcp-turbo PRIVATE 
    $<$<CONFIG:Release>:-O3 -march=native -flto>
    -Wall -Wextra
)