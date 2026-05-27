#include <gtest/gtest.h>

#include "primitive/modint.hpp"

TEST(ModIntArithmeticTest, Addition) {
  ModInt<7> a(1);
  ModInt<7> b(1);

  ModInt<7> c = a + b;

  EXPECT_EQ(static_cast<ModInt<7>::raw_value_type>(c), 2);
}

TEST(ModIntArithmeticTest, Addition_WrapsIntoModIntInterval) {
  ModInt<7> a(2);
  ModInt<7> b(ModInt<7>::MOD - 1);

  ModInt<7> c = a + b;

  EXPECT_EQ(static_cast<ModInt<7>::raw_value_type>(c), 1);
}

TEST(ModIntArithmeticTest, Subtraction) {
  ModInt<7> a(5);
  ModInt<7> b(2);

  ModInt<7> c = a - b;

  EXPECT_EQ(static_cast<ModInt<7>::raw_value_type>(c), 3);
}

TEST(ModIntArithmeticTest, Subtraction_WrapsIntoModIntInterval) {
  ModInt<7> a(2);
  ModInt<7> b(5);

  ModInt<7> c = a - b;

  EXPECT_EQ(static_cast<ModInt<7>::raw_value_type>(c), 4);
}