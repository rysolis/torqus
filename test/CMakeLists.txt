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
    ppv-libs
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
    ppv-libs
    Boost::headers
    GTest::gtest
    GTest::gtest_main
)

# Lets test code #include "adapter/..." the same clean way production code
# under example-napi/ and wasm/ reaches those entities -- adapter/ isn't part of
# ppv-libs's own public include dir (include/), since it's the
# binding-agnostic entity layer example-napi/wasm sit on top of, not the core
# library itself.
target_include_directories(test-${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

gtest_discover_tests(test-${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")

  target_compile_options(test-${PROJECT_NAME}
    PRIVATE
      ${STRICT_WARNINGS}
  )

endif()

# The TFHE runtime tests (bootstrap/keyswitch loops, boost::multiprecision
# noise tracking) are unusably slow at -O0. Optimize this target even under
# the Debug preset -- Debug's default flags don't define NDEBUG, so
# assert(bound < 0.25) in evaluator.hpp and friends stay active; only
# codegen speed changes. Flip this off when you actually need to single-step
# through a test in a debugger, where O0 keeps variables inspectable.
option(TFHE_TEST_FAST_DEBUG "Optimize test-${PROJECT_NAME} under Debug builds for speed, at the cost of debuggability" ON)

if(TFHE_TEST_FAST_DEBUG)

  target_compile_options(test-${PROJECT_NAME}
    PRIVATE
      $<$<CONFIG:Debug>:-O2>
  )

endif()