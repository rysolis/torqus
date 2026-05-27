find_package(GTest REQUIRED)

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
    GTest::gtest
    GTest::gtest_main
)

gtest_discover_tests(test-${PROJECT_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")

  target_compile_options(test-${PROJECT_NAME}
    PRIVATE
      ${STRICT_WARNINGS}
  )

endif()