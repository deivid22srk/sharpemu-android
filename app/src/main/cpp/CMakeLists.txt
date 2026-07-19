cmake_minimum_required(VERSION 3.22)
project(SharpEmuNative CXX C ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(FEX_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../fex-emu)

add_compile_options(-fno-strict-aliasing -fno-exceptions)

set(ARCHITECTURE_arm64 1)

add_subdirectory(${FEX_ROOT}/FEXCore ${CMAKE_BINARY_DIR}/FEXCore)

add_library(sharpemu_native SHARED
    fex_bridge.cpp
    sharpemu_syscall_handler.cpp
    sharpemu_memory.cpp
)

target_include_directories(sharpemu_native PRIVATE
    ${FEX_ROOT}/FEXCore/include
    ${FEX_ROOT}/FEXCore/Source
    ${FEX_ROOT}/External/fmt/include
    ${FEX_ROOT}/External/xxhash
    ${FEX_ROOT}/External/unordered_dense/include
    ${FEX_ROOT}/External/vixl/src
    ${FEX_ROOT}/FEXHeaderUtils/include
    ${CMAKE_BINARY_DIR}/FEXCore/generated
)

target_link_libraries(sharpemu_native
    FEXCore
    log
    android
)

target_compile_definitions(sharpemu_native PRIVATE
    ARCHITECTURE_arm64=1
    _GNU_SOURCE
)
