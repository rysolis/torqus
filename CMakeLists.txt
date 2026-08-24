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
# SIMD lane
# --------------------------------------------
# algebra/detail/simd_ops.hpp picks its NEON vs. scalar Lane strategy at
# compile time from __ARM_NEON (compiler-predefined on NEON-capable
# targets) together with PPV_DISABLE_SIMD, which this option controls --
# OFF forces every ppv-libs consumer (tests, native/example N-API/WASM builds) onto
# the portable scalar path even when the compiler could do NEON. Useful to
# A/B the NEON add/sub path, or as an escape hatch if it ever misbehaves on
# a given toolchain.
option(PPV_ENABLE_SIMD "Use ARM NEON vector instructions for Poly/Vector add-sub where available (algebra/detail/simd_ops.hpp)" ON)

if(NOT PPV_ENABLE_SIMD)
  target_compile_definitions(ppv-libs INTERFACE PPV_DISABLE_SIMD)
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#if !defined(__ARM_NEON)
#error no NEON
#endif
int main() { return 0; }
" PPV_HAS_ARM_NEON)

if(PPV_ENABLE_SIMD AND PPV_HAS_ARM_NEON)
  message(STATUS "SIMD lane for Poly/Vector add-sub (ModInt/ModTorus): NEON (uint32x4, width 4)")
elseif(PPV_HAS_ARM_NEON)
  message(STATUS "SIMD lane for Poly/Vector add-sub (ModInt/ModTorus): Scalar (width 1) -- NEON available but disabled via -DPPV_ENABLE_SIMD=OFF")
else()
  message(STATUS "SIMD lane for Poly/Vector add-sub (ModInt/ModTorus): Scalar (width 1, __ARM_NEON not defined)")
endif()

# --------------------------------------------
# Fresh-encryption noise
# --------------------------------------------
# tfhe/params.hpp's alpha_of<Params> normally returns a param set's
# noise_params<AlphaBits>::alpha (0 if none was mixed in), which
# tlwe_cryptor.hpp/trlwe_cryptor.hpp/trgsw_cryptor.hpp use to sample real
# Gaussian noise at encryption (see noise.hpp's fresh_noise_bound for the
# matching analytic tracker side). OFF forces alpha_of<Params>::value to 0
# for every Params regardless of noise_params, i.e. every ppv-libs consumer
# reverts to exact/noiseless ciphertexts -- an escape hatch for debugging
# against the old noiseless behavior without editing every param set.
option(TFHE_ENABLE_NOISE "Sample real Gaussian noise at TLWE/TRLWE/TRGSW encryption for param sets that opt in via noise_params" ON)

if(NOT TFHE_ENABLE_NOISE)
  target_compile_definitions(ppv-libs INTERFACE TFHE_DISABLE_NOISE)
endif()

# --------------------------------------------
# Threading
# --------------------------------------------
# tfhe/utility/thread_pool.hpp (ThreadPool) is std::thread-based, used e.g.
# by example/protocol/downstream.hpp to parallelize seal() across
# candidates. Not needed under Emscripten: Downstream sizes its pool to
# 0 there (no -pthread in wasm/CMakeLists.txt), so ThreadPool never spawns
# a std::thread on that path.
if(NOT EMSCRIPTEN)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(ppv-libs INTERFACE Threads::Threads)
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