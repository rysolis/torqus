#include <gtest/gtest.h>

#include "primitive/torus.hpp"

namespace torus_conversion_test {
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
};  // namespace torus_conversion_test

template <typename Ctx>
class ModTorusConvertTest : public ::testing::Test {};

TYPED_TEST_SUITE(ModTorusConvertTest, torus_conversion_test::TestContexts);

TYPED_TEST(ModTorusConvertTest, ConvertTorusLValue2ModTorus) {
  using Torus = typename TypeParam::torus_type;

  detail::Torus dt(0.5);
  Torus mt = static_cast<Torus>(dt);

  EXPECT_EQ((1 << (Torus::qbit - 1)), static_cast<Torus::raw_value_type>(mt));
}

TYPED_TEST(ModTorusConvertTest, ConvertTorusRValue2ModTorus) {
  using Torus = typename TypeParam::torus_type;

  Torus mt = static_cast<Torus>(detail::Torus(0.5));

  EXPECT_EQ((1 << (Torus::qbit - 1)), static_cast<Torus::raw_value_type>(mt));
}

// Convert ModTorus to Torus and back, and check if the value is
// preserved
TYPED_TEST(ModTorusConvertTest, ConvertModTorusLValue2Torus) {
  using Torus = typename TypeParam::torus_type;

  Torus mt(1 << (Torus::qbit - 1));
  detail::Torus dt = static_cast<detail::Torus>(mt);

  EXPECT_NEAR(0.5, static_cast<detail::Torus::raw_value_type>(dt),
              std::numeric_limits<double>::epsilon());
}

TYPED_TEST(ModTorusConvertTest, ConvertModTorusRValue2Torus) {
  using Torus = typename TypeParam::torus_type;

  detail::Torus dt = static_cast<detail::Torus>(Torus(1 << (Torus::qbit - 1)));

  EXPECT_NEAR(0.5, static_cast<detail::Torus::raw_value_type>(dt),
              std::numeric_limits<double>::epsilon());
}