#include <gtest/gtest.h>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

TEST(TorusArithmeticTest, Addition_WrapsIntoUnitInterval) {
  Torus a(0.75);
  Torus b(0.50);

  Torus c = a + b;

  EXPECT_NEAR(static_cast<Torus::raw_value_type>(c), 0.25, 1e-9);
}

TEST(TorusArithmeticTest, Subtraction_WrapsIntoUnitInterval) {
  Torus a(0.25);
  Torus b(0.50);

  Torus c = a - b;

  EXPECT_NEAR(static_cast<Torus::raw_value_type>(c), 0.75, 1e-9);
}

TEST(TorusArithmeticTest, ScalarMultiplication_WrapsCorrectly) {
  Torus t(0.2);

  Torus result = UInt(7) * t;

  EXPECT_NEAR(static_cast<Torus::raw_value_type>(result), 0.4, 1e-9);
}