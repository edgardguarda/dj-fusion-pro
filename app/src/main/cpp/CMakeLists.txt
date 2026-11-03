cmake_minimum_required(VERSION 3.22.1)
project(djfusion LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Oboe vem do Maven pelo Prefab (build.gradle.kts: prefab = true).
find_package(oboe REQUIRED CONFIG)

add_library(djfusion SHARED
    native_bridge.cpp
    core/engine.cpp)

target_include_directories(djfusion PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(djfusion PRIVATE -O3 -Wall -Wextra)
target_link_options(djfusion PRIVATE -Wl,-z,max-page-size=16384)
target_link_libraries(djfusion oboe::oboe android log)
