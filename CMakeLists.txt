cmake_minimum_required(VERSION 3.20)
project(AED2)

set(CMAKE_CXX_STANDARD 23)

set_source_files_properties(Arvoreb.cppm PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(AED2
        Arvoreb.cppm
        main.cpp
)
