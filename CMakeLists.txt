cmake_minimum_required(VERSION 3.20)
project(AED2)

set(CMAKE_CXX_STANDARD 23)

add_executable(AED2
    include/ArvoreB.hpp
    src/ArvoreB.cpp
    src/main.cpp
)

target_include_directories(AED2 PRIVATE include)
