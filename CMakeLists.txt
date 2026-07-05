cmake_minimum_required(VERSION 3.20)
project(graph_tasks)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(graph_common INTERFACE)
target_include_directories(graph_common INTERFACE common)

add_library(project_options INTERFACE)

target_compile_options(project_options INTERFACE
        -Wall -Wextra -Wpedantic -Werror
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
)

target_link_options(project_options INTERFACE
        -fsanitize=address,undefined
)

add_executable(task1_1 task1.1/main.cpp)
target_link_libraries(task1_1 PRIVATE graph_common project_options)

add_executable(task1_2 task1.2/main.cpp)
target_link_libraries(task1_2 PRIVATE graph_common project_options)

add_executable(task1_3 task1.3/main.cpp)
target_link_libraries(task1_3 PRIVATE graph_common project_options)

add_executable(task1_4 task1.4/main.cpp)
target_link_libraries(task1_4 PRIVATE graph_common project_options)