#include <gtest/gtest.h>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

TEST(TorusArithmeticTest, Addition_WrapsIntoUnitInterval) {
  detail::Torus a(0.75), b(0.50);
  detail::Torus c = a + b;
  EXPECT_NEAR(0.25, static_cast<detail::Torus::raw_value_type>(c),
              std::numeric_limits<double>::epsilon());
}

TEST(TorusArithmeticTest, Subtraction_WrapsIntoUnitInterval) {
  detail::Torus a(0.25), b(0.50);
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

namespace modtorus_arithmetic_test {
struct Ctx1 {
  using torus_type = ModTorus<8>;
};
struct Ctx2 {
  using torus_type = ModTorus<16>;
};
struct Ctx3 {
  using torus_type = ModTorus<32>;
};
using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3>;
}  // namespace modtorus_arithmetic_test

template <typename Ctx>
class ModTorusArithmeticTest : public ::testing::Test {};

TYPED_TEST_SUITE(ModTorusArithmeticTest,
                 modtorus_arithmetic_test::TestContexts);

TYPED_TEST(ModTorusArithmeticTest, Addition_WrapsIntoModTorusInterval) {
  using Torus = typename TypeParam::torus_type;
  Torus a(Torus::raw_max()), b(2);
  Torus c = a + b;
  EXPECT_EQ(1, static_cast<typename Torus::raw_value_type>(c));
}

TYPED_TEST(ModTorusArithmeticTest, Subtraction_WrapsIntoModTorusInterval) {
  using Torus = typename TypeParam::torus_type;
  Torus a(2), b(3);
  Torus c = a - b;
  EXPECT_EQ(Torus::raw_max(), static_cast<Torus::raw_value_type>(c));
}

TYPED_TEST(ModTorusArithmeticTest, AdditionAssignment) {
  using Torus = typename TypeParam::torus_type;
  Torus a(3);
  a += Torus(4);
  EXPECT_EQ(7, static_cast<typename Torus::raw_value_type>(a));
}