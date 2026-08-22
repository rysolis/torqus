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
# Allocator override
# --------------------------------------------
# algebra/container.hpp heap-allocates its fixed-size buffer on every
# construction (see Container::storage_) so that moves stay an O(1) pointer
# swap and interpret_as() can hand a buffer to another Poly/Vector type
# without copying it. That trades a malloc/free per Container for a cheap
# move, which is worth it as long as the allocator itself is fast. mimalloc
# gives every executable linking ppv-libs that allocator, for free, with no
# change to Container. Not available under Emscripten, which brings its own.
option(PPV_USE_MIMALLOC "Override the global allocator with mimalloc" ON)

if(PPV_USE_MIMALLOC AND NOT EMSCRIPTEN)
  find_package(mimalloc CONFIG QUIET)
  if(mimalloc_FOUND)
    message(STATUS "mimalloc found: overriding the global allocator for ppv-libs consumers")
    target_link_libraries(ppv-libs INTERFACE mimalloc)
  else()
    message(STATUS "mimalloc not found: falling back to the system allocator (install libmimalloc-dev, or the mimalloc Homebrew formula, to enable)")
  endif()
endif()

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