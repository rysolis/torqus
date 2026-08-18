cmake_minimum_required(VERSION 3.22)
# --------------------------------------------
# Project
# --------------------------------------------
set(PROJECT_NAME ppv-lab)
project(${PROJECT_NAME} LANGUAGES CXX)
set(WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------
# Library
# --------------------------------------------

add_library(ppv-libs INTERFACE)

target_include_directories(ppv-libs
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --------------------------------------------
# Emscripten specific
# --------------------------------------------

if(EMSCRIPTEN)
  message(STATUS "Building with Emscripten")
  add_subdirectory(wasm)
endif()


# --------------------------------------------
# test
# --------------------------------------------
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()