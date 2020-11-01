cmake_minimum_required(VERSION 3.5)

add_subdirectory(ext/cppast)

project(rtjsgen LANGUAGES CXX)

find_package(Qt5 COMPONENTS Core)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_AUTOMOC TRUE)
set(CMAKE_AUTORCC TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(rtjsgen main.cpp res.qrc)

target_link_libraries(rtjsgen
  PUBLIC
    Qt5::Core
    cppast
)

install(TARGETS rtjsgen)
