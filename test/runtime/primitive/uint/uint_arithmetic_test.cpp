#include <gtest/gtest.h>

#include "primitive/uint.hpp"

TEST(UIntArithmeticTest, AdditionCorrectness) {
  UInt a(100), b(33);
  UInt c = a + b;
  EXPECT_EQ(UInt(133), c);
  EXPECT_EQ(UInt(100), a);
  EXPECT_EQ(UInt(33), b);
}

TEST(UIntArithmeticTest, Addition_Overflow) {
  UInt a(1), b(std::numeric_limits<UInt::raw_value_type>::max());
  EXPECT_THROW({ a + b; }, std::overflow_error);
}

TEST(UIntArithmeticTest, SubtractionCorrectness) {
  UInt a(100), b(2);
  UInt c = a - b;
  EXPECT_EQ(UInt(98), c);
  EXPECT_EQ(UInt(100), a);
  EXPECT_EQ(UInt(2), b);
}

TEST(UIntArithmeticTest, SubtractionUnderflow) {
  UInt a(1), b(2);
  EXPECT_THROW({ a - b; }, std::underflow_error);
}