#include <gtest/gtest.h>

#include "primitive/torus.hpp"

namespace torus_conversion_test {
struct Ctx1 {
  using Torus = ModTorus<16>;
};
}  // namespace torus_conversion_test

template <typename Ctx>
class ModTorusConvertTest : public ::testing::Test {};

using TestContexts = ::testing::Types<torus_conversion_test::Ctx1>;

TYPED_TEST_SUITE(ModTorusConvertTest, TestContexts);

TYPED_TEST(ModTorusConvertTest, ConvertTorusLValue2ModTorus) {
  using Torus = typename TypeParam::Torus;

  detail::Torus dt(0.5);
  Torus mt = static_cast<Torus>(dt);

  EXPECT_EQ(static_cast<Torus::raw_value_type>(mt), Torus::q / 2);
}

TYPED_TEST(ModTorusConvertTest, ConvertTorusRValue2ModTorus) {
  using Torus = typename TypeParam::Torus;

  Torus mt = static_cast<Torus>(detail::Torus(0.5));

  EXPECT_EQ(static_cast<Torus::raw_value_type>(mt), Torus::q / 2);
}

// Convert ModTorus to Torus and back, and check if the value is
// preserved
TYPED_TEST(ModTorusConvertTest, ConvertModTorusLValue2Torus) {
  using Torus = typename TypeParam::Torus;

  Torus mt(Torus::q / 2);
  detail::Torus dt = static_cast<detail::Torus>(mt);

  EXPECT_NEAR(static_cast<detail::Torus::raw_value_type>(dt), 0.5, 1e-9);
}

TYPED_TEST(ModTorusConvertTest, ConvertModTorusRValue2Torus) {
  using Torus = typename TypeParam::Torus;

  detail::Torus dt = static_cast<detail::Torus>(Torus(Torus::q / 2));

  EXPECT_NEAR(static_cast<detail::Torus::raw_value_type>(dt), 0.5, 1e-9);
}