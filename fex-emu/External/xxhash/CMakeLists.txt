cmake_minimum_required(VERSION 3.10)
project(xxhash C)

add_library(xxhash STATIC xxhash.c)
target_include_directories(xxhash PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
