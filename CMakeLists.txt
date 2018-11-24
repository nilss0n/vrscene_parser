cmake_minimum_required(VERSION 3.0.0)
project(vrscene_parser VERSION 0.1.0)

set (CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra")
add_executable(vrscene_parser main.cpp vrscene_parser.h)