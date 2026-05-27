#include <gtest/gtest.h>

#include "primitive/uint.hpp"

TEST(UIntArithmeticTest, AdditionCorrectness) {
  UInt a(100), b(33);
  UInt c = a + b;
  EXPECT_EQ(c, UInt(133));
}

TEST(UIntArithmeticTest, Addition_Overflow) {
  UInt a(1), b(std::numeric_limits<UInt::raw_value_type>::max());
  EXPECT_THROW({ a + b; }, std::overflow_error);
}

TEST(UIntArithmeticTest, SubtractionCorrectness) {
  UInt a(100), b(2);
  UInt c = a - b;
  EXPECT_EQ(c, UInt(98));
  EXPECT_EQ(a, UInt(100));
  EXPECT_EQ(b, UInt(2));
}

TEST(UIntArithmeticTest, SubtractionUnderflow) {
  UInt a(1), b(2);
  EXPECT_THROW({ a - b; }, std::underflow_error);
  EXPECT_EQ(a, UInt(1));
  EXPECT_EQ(b, UInt(2));
}