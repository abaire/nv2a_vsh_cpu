cmake_minimum_required(VERSION 3.18)

project(nv2a_vsh_cpu)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

enable_testing()
set(CMAKE_CXX_STANDARD 17)
include(CMakePushCheckState)
set(_CMAKE_PROCESSING_LANGUAGE "C")
include(CheckSymbolExists)
include (ExternalProject)
include(FindPkgConfig)

find_package(
        Boost 1.70
        COMPONENTS
        unit_test_framework
        REQUIRED
)

set(GENERATED_FILES_DIR "${CMAKE_BINARY_DIR}/generated")
include_directories("${Boost_INCLUDE_DIR}" "${GENERATED_FILES_DIR}")

set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# xbdm executable ------------------------------------

add_library(
        nv2a_vsh_cpu
        src/nv2a_vsh_cpu.c
        src/nv2a_vsh_cpu.h
)

target_include_directories(
        nv2a_vsh_cpu
        PRIVATE
        src
)

# Tests ----------------------------------------------
add_executable(
        operations_tests
        test/operations/test_main.cpp
        test/operations/test_basic.cpp
)
target_include_directories(
        operations_tests
        PRIVATE src
        PRIVATE test
)
target_link_libraries(
        operations_tests
        LINK_PRIVATE
        nv2a_vsh_cpu
        ${Boost_LIBRARIES}
)
add_test(NAME operations_tests COMMAND operations_tests)
add_dependencies(operations_tests nv2a_vsh_cpu)
