cmake_minimum_required(VERSION 4.0)
project(untitled)
target_include_directories(untitled PRIVATE ${CMAKE_SOURCE_DIR}/third_party/stb)

set(CMAKE_CXX_STANDARD 20)

add_executable(untitled main.cpp)
