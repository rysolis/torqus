cmake_minimum_required(VERSION 3.22)
# --------------------------------------------
# Project
# --------------------------------------------
set(PROJECT_NAME torqus)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES CXX)
set(WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)

# --------------------------------------------
# Library
# --------------------------------------------

add_library(torqus-libs INTERFACE)
add_library(torqus::torqus ALIAS torqus-libs)

# CMAKE_CXX_STANDARD above only sets the default for targets in *this*
# build -- an installed torqus::torqus needs its own usage requirement so a
# consumer that hasn't set C++20 itself still gets it (this header-only
# library uses concepts/ranges throughout).
target_compile_features(torqus-libs INTERFACE cxx_std_20)

target_include_directories(torqus-libs
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# --------------------------------------------
# Allocator override
# --------------------------------------------
# algebra/container.hpp heap-allocates its fixed-size buffer on every
# construction (see Container::storage_) so that moves stay an O(1) pointer
# swap and interpret_as() can hand a buffer to another Poly/Vector type
# without copying it. That trades a malloc/free per Container for a cheap
# move, which is worth it as long as the allocator itself is fast. mimalloc
# gives every executable linking torqus-libs that allocator, for free, with no
# change to Container. Not available under Emscripten, which brings its own.
option(TORQUS_USE_MIMALLOC "Override the global allocator with mimalloc" ON)

if(TORQUS_USE_MIMALLOC AND NOT EMSCRIPTEN)
  find_package(mimalloc CONFIG QUIET)
  if(mimalloc_FOUND)
    message(STATUS "mimalloc found: overriding the global allocator for torqus-libs consumers")
    target_link_libraries(torqus-libs INTERFACE mimalloc)
  else()
    message(STATUS "mimalloc not found: falling back to the system allocator (install libmimalloc-dev, or the mimalloc Homebrew formula, to enable)")
  endif()
endif()

# --------------------------------------------
# SIMD lane
# --------------------------------------------
# algebra/detail/simd_ops.hpp picks its NEON vs. scalar Lane strategy at
# compile time from __ARM_NEON (compiler-predefined on NEON-capable
# targets) together with TORQUS_DISABLE_SIMD, which this option controls --
# OFF forces every torqus-libs consumer (this repo's own tests, or any
# downstream native/example N-API/WASM build) onto the portable scalar path even
# when the compiler could do NEON. Useful to
# A/B the NEON add/sub path, or as an escape hatch if it ever misbehaves on
# a given toolchain.
option(TORQUS_ENABLE_SIMD "Use ARM NEON vector instructions for Poly/Vector add-sub where available (algebra/detail/simd_ops.hpp)" ON)

if(NOT TORQUS_ENABLE_SIMD)
  target_compile_definitions(torqus-libs INTERFACE TORQUS_DISABLE_SIMD)
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#if !defined(__ARM_NEON)
#error no NEON
#endif
int main() { return 0; }
" TORQUS_HAS_ARM_NEON)

if(TORQUS_ENABLE_SIMD AND TORQUS_HAS_ARM_NEON)
  message(STATUS "SIMD lane for Poly/Vector add-sub (ModInt/ModTorus): NEON (uint32x4, width 4)")
elseif(TORQUS_HAS_ARM_NEON)
  message(STATUS "SIMD lane for Poly/Vector add-sub (ModInt/ModTorus): Scalar (width 1) -- NEON available but disabled via -DTORQUS_ENABLE_SIMD=OFF")
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
# for every Params regardless of noise_params, i.e. every torqus-libs consumer
# reverts to exact/noiseless ciphertexts -- an escape hatch for debugging
# against the old noiseless behavior without editing every param set.
option(TFHE_ENABLE_NOISE "Sample real Gaussian noise at TLWE/TRLWE/TRGSW encryption for param sets that opt in via noise_params" ON)

if(NOT TFHE_ENABLE_NOISE)
  target_compile_definitions(torqus-libs INTERFACE TFHE_DISABLE_NOISE)
endif()

# --------------------------------------------
# Threading
# --------------------------------------------
# tfhe/utility/thread_pool.hpp (ThreadPool) is std::thread-based, for
# consumers that want to parallelize work across ciphertexts/entities of
# their own. Not linked under Emscripten -- a wasm consumer that wants a
# thread pool sizes and configures it on its own build's terms (e.g.
# -pthread / -sPROXY_TO_PTHREAD), which is outside this header-only
# library's own build.
if(NOT EMSCRIPTEN)
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(torqus-libs INTERFACE Threads::Threads)
endif()

# --------------------------------------------
# Install / package export (Conan, vcpkg, find_package(torqus))
# --------------------------------------------
# torqus-libs is header-only, so packaging it is just: install the headers,
# then export torqus-libs under the public name torqus::torqus so a consumer's
# find_package(torqus CONFIG) + target_link_libraries(... torqus::torqus) sees
# the same include dirs + Threads/mimalloc usage requirements set up
# above. Skipped under Emscripten -- a wasm build isn't something a native
# find_package(torqus) consumer would ever link against, and this repo
# doesn't produce one itself (downstream consumers building torqus under
# Emscripten do so via the plain CMake/Conan/vcpkg paths above instead).
if(NOT EMSCRIPTEN)
  set_target_properties(torqus-libs PROPERTIES EXPORT_NAME torqus)

  install(TARGETS torqus-libs EXPORT torqusTargets)

  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(EXPORT torqusTargets
    FILE torqusTargets.cmake
    NAMESPACE torqus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/torqus
  )

  include(CMakePackageConfigHelpers)

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/torqusConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/torqusConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/torqus
  )

  # Header-only and not tied to any particular ABI, so consumers on a
  # different architecture/compiler than the one that ran `install` can
  # still use this config -- ARCH_INDEPENDENT skips the arch check
  # write_basic_package_version_file would otherwise add.
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/torqusConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/torqusConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/torqusConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/torqus
  )
endif()

# --------------------------------------------
# test
# --------------------------------------------
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()