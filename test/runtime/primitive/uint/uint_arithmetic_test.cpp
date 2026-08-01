#include <gtest/gtest.h>

#include "primitive/uint.hpp"

namespace uint_arithmetic_test {
using TestContexts = ::testing::Types<UInt>;
}  // namespace uint_arithmetic_test

class UIntArithmeticTest : public ::testing::Test {
 protected:
  struct TestCase {
    UInt lhs;
    UInt rhs;
  };

  [[nodiscard]] std::vector<TestCase> addition_valid_cases() {
    return {
        {UInt(1), UInt(2)},
        {UInt(0), UInt(0)},
        {UInt(UInt::raw_max()), UInt(0)},
        {UInt(UInt::raw_max() - 1), UInt(1)},
    };
  }

  [[nodiscard]] std::vector<TestCase> addition_invalid_cases() {
    return {
        {UInt(UInt::raw_max()), UInt(1)},
    };
  }

  [[nodiscard]] std::vector<TestCase> subtraction_valid_cases() {
    return {
        {UInt(3), UInt(1)},
        {UInt(0), UInt(0)},
        {UInt(UInt::raw_max()), UInt(1)},
    };
  }

  [[nodiscard]] std::vector<TestCase> subtraction_invalid_cases() {
    return {
        {UInt(1), UInt(3)},
    };
  }
};

TEST_F(UIntArithmeticTest, Addition_Correctness) {
  for (const auto& tc : this->addition_valid_cases()) {
    UInt lhs = tc.lhs;
    UInt rhs = tc.rhs;
    UInt res = lhs + rhs;
    EXPECT_EQ(static_cast<UInt::raw_value_type>(lhs + rhs),
              static_cast<UInt::raw_value_type>(res));
  }
}

TEST_F(UIntArithmeticTest, Addition_Overflow) {
  for (const auto& tc : this->addition_invalid_cases()) {
    UInt lhs = tc.lhs;
    UInt rhs = tc.rhs;
    EXPECT_THROW({ lhs + rhs; }, std::overflow_error);
  }
}

TEST_F(UIntArithmeticTest, SubtractionCorrectness) {
  for (const auto& tc : this->subtraction_valid_cases()) {
    UInt lhs = tc.lhs;
    UInt rhs = tc.rhs;
    UInt res = lhs - rhs;
    EXPECT_EQ(static_cast<UInt::raw_value_type>(lhs - rhs),
              static_cast<UInt::raw_value_type>(res));
  }
}

TEST_F(UIntArithmeticTest, SubtractionUnderflow) {
  for (const auto& tc : this->subtraction_invalid_cases()) {
    UInt lhs = tc.lhs;
    UInt rhs = tc.rhs;
    EXPECT_THROW({ lhs - rhs; }, std::underflow_error);
  }
}