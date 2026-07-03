cmake_minimum_required(VERSION 3.20)
project(task1.1)

set(CMAKE_CXX_STANDARD 23)

add_executable(task1 main.cpp
        types.h
        vertex.h
        edge.h
        graph.h)
