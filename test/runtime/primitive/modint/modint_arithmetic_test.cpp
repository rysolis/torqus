#include <gtest/gtest.h>

#include "primitive/modint.hpp"

TEST(ModIntArithmeticTest, Addition) {
  ModInt<7> a(1);
  ModInt<7> b(1);

  ModInt<7> c = a + b;

  EXPECT_EQ(2, static_cast<ModInt<7>::raw_value_type>(c));
}

TEST(ModIntArithmeticTest, Addition_WrapsIntoModIntInterval) {
  ModInt<7> a(2);
  ModInt<7> b(ModInt<7>::MOD - 1);

  ModInt<7> c = a + b;

  EXPECT_EQ(1, static_cast<ModInt<7>::raw_value_type>(c));
}

TEST(ModIntArithmeticTest, Subtraction) {
  ModInt<7> a(5);
  ModInt<7> b(2);

  ModInt<7> c = a - b;

  EXPECT_EQ(3, static_cast<ModInt<7>::raw_value_type>(c));
}

TEST(ModIntArithmeticTest, Subtraction_WrapsIntoModIntInterval) {
  ModInt<7> a(2);
  ModInt<7> b(5);

  ModInt<7> c = a - b;

  EXPECT_EQ(4, static_cast<ModInt<7>::raw_value_type>(c));
}