find_package(GTest REQUIRED)

# Boost.Rational + Boost.Multiprecision (header-only) back the exact-rational
# noise bound computation in tfhe/utility/analysis/exact_bound.hpp, which is
# only pulled in via tfhe/operation/evaluator.hpp -- included by the runtime
# tests below, not by any production/wasm code path.
find_package(Boost REQUIRED)

# --------------------------------------------
# warnings
# --------------------------------------------

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")

  set(STRICT_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wsign-conversion
  )

endif()

# --------------------------------------------
# compile-only interface tests
# --------------------------------------------

file(GLOB_RECURSE
  INTERFACE_TEST_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/compile/*_test.cpp
)

add_library(compile-test OBJECT
  ${INTERFACE_TEST_SOURCE_FILES}
)

target_link_libraries(compile-test
  PRIVATE
    torqus-libs
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")

  target_compile_options(compile-test
    PRIVATE
      ${STRICT_WARNINGS}
  )

endif()

# --------------------------------------------
# runtime tests
# --------------------------------------------

file(GLOB_RECURSE
  TEST_SOURCE_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/runtime/*_test.cpp
)

add_executable(test-${PROJECT_NAME}
  ${TEST_SOURCE_FILES}
)

target_link_libraries(test-${PROJECT_NAME}
  PRIVATE
    torqus-libs
    Boost::headers
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(test-${PROJECT_NAME})

# Set by the *-native-* CMake presets (CMakePresets.json's native-base),
# which add -march=native: the resulting binary is tied to the build
# machine's CPU features, so name it after the host architecture rather
# than letting it look like an interchangeable, portable test-torqus.
option(TORQUS_NATIVE_BUILD "Building with -march=native -- names the test binary after the host CPU architecture" OFF)

if(TORQUS_NATIVE_BUILD)

  set_target_properties(test-${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "test-${PROJECT_NAME}-native-${CMAKE_SYSTEM_PROCESSOR}"
  )

endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")

  target_compile_options(test-${PROJECT_NAME}
    PRIVATE
      ${STRICT_WARNINGS}
  )

endif()

# The TFHE runtime tests (bootstrap/keyswitch loops, boost::multiprecision
# noise tracking) are unusably slow at -O0. Optimize this target even under
# the Debug preset -- Debug's default flags don't define NDEBUG, so
# invariant checks like assert(other.size() == this->size()) (algebra/vector.hpp)
# and friends stay active; only codegen speed changes. Flip this off when
# you actually need to single-step through a test in a debugger, where O0
# keeps variables inspectable.
option(TFHE_TEST_FAST_DEBUG "Optimize test-${PROJECT_NAME} under Debug builds for speed, at the cost of debuggability" ON)

if(TFHE_TEST_FAST_DEBUG)

  target_compile_options(test-${PROJECT_NAME}
    PRIVATE
      $<$<CONFIG:Debug>:-O2>
  )

endif()