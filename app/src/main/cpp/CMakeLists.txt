cmake_minimum_required(VERSION 3.22)
project(SharpEmuNative CXX C ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT DEFINED FEX_ROOT)
    get_filename_component(FEX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../fex-emu" REALPATH)
endif()

add_compile_options(-fno-strict-aliasing -fno-exceptions)

set(ARCHITECTURE_arm64 1)
set(BUILD_TESTING OFF)
set(ENABLE_LTO OFF)
set(ENABLE_FEX_ALLOCATOR OFF)
set(ENABLE_JEMALLOC_GLIBC_ALLOC OFF)
set(BUILD_FEXCONFIG OFF)
set(BUILD_THUNKS OFF)
set(BUILD_FEX_LINUX_TESTS OFF)
set(ENABLE_GDB_SYMBOLS OFF)
set(ENABLE_OFFLINE_TELEMETRY OFF)
set(ENABLE_FEXCORE_PROFILER OFF)
set(BUILD_STEAM_SUPPORT OFF)

list(APPEND CMAKE_MODULE_PATH "${FEX_ROOT}/Data/CMake")

include(LinkerGC)

add_subdirectory(${FEX_ROOT}/External/fmt ${CMAKE_BINARY_DIR}/fmt)
add_subdirectory(${FEX_ROOT}/External/xxhash ${CMAKE_BINARY_DIR}/xxhash)
add_subdirectory(${FEX_ROOT}/External/unordered_dense ${CMAKE_BINARY_DIR}/unordered_dense)
add_subdirectory(${FEX_ROOT}/External/vixl ${CMAKE_BINARY_DIR}/vixl)
add_subdirectory(${FEX_ROOT}/External/cephes ${CMAKE_BINARY_DIR}/cephes)
add_subdirectory(${FEX_ROOT}/FEXHeaderUtils ${CMAKE_BINARY_DIR}/FEXHeaderUtils)
add_subdirectory(${FEX_ROOT}/CodeEmitter ${CMAKE_BINARY_DIR}/CodeEmitter)
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
