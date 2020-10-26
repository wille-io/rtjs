cmake_minimum_required(VERSION 3.5)

add_subdirectory(ext/cppast)

project(rtjs LANGUAGES CXX)

find_package(Qt5 COMPONENTS Core)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_AUTOMOC TRUE)

add_executable(rtjs main.cpp)

target_link_libraries(rtjs
  PUBLIC
    Qt5::Core
    cppast
)
