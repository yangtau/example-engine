cmake_minimum_required(VERSION 3.10)
project(Project)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Project
        block.cpp
        block.h
        btree.cpp
        btree.h
        storage.cpp
        storage.h
        file.cpp
        file.h
        main.cpp buffer.cpp buffer.h)
