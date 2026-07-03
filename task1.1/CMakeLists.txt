cmake_minimum_required(VERSION 3.20)
project(task1.1)

set(CMAKE_CXX_STANDARD 23)

add_executable(task1 main.cpp
        types.hpp
        vertex.hpp
        edge.hpp
        graph.hpp
        io.hpp)
