cmake_minimum_required(VERSION 3.5)

project(TestTarget LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(rtjs)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(TestTarget main.cpp x.cpp x.h)

RtjsTarget(TestTarget)

target_link_libraries(TestTarget
  jerry-core
  jerry-port-default
  pthread
)
