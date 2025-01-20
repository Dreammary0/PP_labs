cmake_minimum_required(VERSION 3.29)
project(lab1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-mavx -pthread -O3 -fopenmp")

add_executable(lab1 main.cpp)
