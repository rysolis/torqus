#include <gtest/gtest.h>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

TEST(TorusArithmeticTest, Addition_WrapsIntoUnitInterval) {
  detail::Torus a(0.75);
  detail::Torus b(0.50);

  detail::Torus c = a + b;

  EXPECT_NEAR(0.25, static_cast<detail::Torus::raw_value_type>(c),
              std::numeric_limits<double>::epsilon());
}

TEST(TorusArithmeticTest, Subtraction_WrapsIntoUnitInterval) {
  detail::Torus a(0.25);
  detail::Torus b(0.50);

  detail::Torus c = a - b;

  EXPECT_NEAR(0.75, static_cast<detail::Torus::raw_value_type>(c),
              std::numeric_limits<double>::epsilon());
}

TEST(TorusArithmeticTest, ScalarMultiplication_WrapsCorrectly) {
  detail::Torus t(0.2);

  detail::Torus result = UInt(7) * t;

  EXPECT_NEAR(0.4, static_cast<detail::Torus::raw_value_type>(result),
              10 * std::numeric_limits<double>::epsilon());
}