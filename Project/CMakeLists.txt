cmake_minimum_required(VERSION 3.10)
project(Project)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Project
        common.h
        block.cpp
        block.h
        btree.cpp
        btree.h
        buffer.cpp
        buffer.h
        file.cpp
        matu.h
        main.cpp)
