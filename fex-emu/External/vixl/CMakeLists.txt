cmake_minimum_required(VERSION 3.10)
project(vixl CXX)

file(GLOB_RECURSE VIXL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)

add_library(vixl STATIC ${VIXL_SOURCES})
target_include_directories(vixl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(vixl PUBLIC cxx_std_17)
