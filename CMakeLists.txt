cmake_minimum_required(VERSION 3.22)
project(sysgraph)

set(CMAKE_CXX_STANDARD 17)

add_executable(sysgraph main.cpp)
target_link_libraries(sysgraph -static)
