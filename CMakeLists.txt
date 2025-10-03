cmake_minimum_required(VERSION 3.22)
project(FalconOCR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
  set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# Options
option(FALCON_BUILD_TESTS "Build unit tests" ON)
option(FALCON_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# Warnings
if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(src)

if(FALCON_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
