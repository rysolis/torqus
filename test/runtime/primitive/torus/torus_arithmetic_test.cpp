#include <gtest/gtest.h>

#include <vector>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

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
// Word pinned explicitly to uint64_t (rather than relying on
// TORQUS_TORUS_BITS's default) so this QBit=64 case -- exercising the
// "qbit spans the full native word width" sentinel path (see
// tfhe/math/modswitch.hpp) -- runs the same way regardless of which
// default this build was configured with.
struct Ctx4 {
  using torus_type = ModTorus<64, uint64_t>;
};
using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3, Ctx4>;
}  // namespace modtorus_arithmetic_test

template <typename Ctx>
class ModTorusArithmeticTest : public ::testing::Test {
 protected:
  using Torus = typename Ctx::torus_type;

  struct TestCase {
    Torus lhs;
    Torus rhs;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {
        {Torus(1), Torus(2)},
        {Torus(Torus::raw_max()), Torus(1)},
        {Torus(0), Torus(Torus::raw_max())},
    };
  }
};

TYPED_TEST_SUITE(ModTorusArithmeticTest,
                 modtorus_arithmetic_test::TestContexts);

TYPED_TEST(ModTorusArithmeticTest, AdditionCorrectness) {
  using Torus = typename TypeParam::torus_type;

  for (const auto& tc : this->cases()) {
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;

    Torus res = lhs + rhs;

    EXPECT_EQ(static_cast<typename Torus::raw_value_type>(res),
              (static_cast<typename Torus::raw_value_type>(tc.lhs) +
               static_cast<typename Torus::raw_value_type>(tc.rhs)) &
                  Torus::mask());
  }
}

TYPED_TEST(ModTorusArithmeticTest, SubtractionCorrectness) {
  using Torus = typename TypeParam::torus_type;

  for (const auto& tc : this->cases()) {
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;

    Torus res = lhs - rhs;

    EXPECT_EQ(static_cast<typename Torus::raw_value_type>(res),
              (static_cast<typename Torus::raw_value_type>(tc.lhs) -
               static_cast<typename Torus::raw_value_type>(tc.rhs)) &
                  Torus::mask());
  }
}

TYPED_TEST(ModTorusArithmeticTest, AdditionAssignmentCorrectness) {
  using Torus = typename TypeParam::torus_type;

  for (const auto& tc : this->cases()) {
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;

    lhs += rhs;

    EXPECT_EQ(static_cast<typename Torus::raw_value_type>(lhs),
              (static_cast<typename Torus::raw_value_type>(tc.lhs) +
               static_cast<typename Torus::raw_value_type>(tc.rhs)) &
                  Torus::mask());
  }
}

TYPED_TEST(ModTorusArithmeticTest, SubtractionAssignmentCorrectness) {
  using Torus = typename TypeParam::torus_type;

  for (const auto& tc : this->cases()) {
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;

    lhs -= rhs;

    EXPECT_EQ(static_cast<typename Torus::raw_value_type>(lhs),
              (static_cast<typename Torus::raw_value_type>(tc.lhs) -
               static_cast<typename Torus::raw_value_type>(tc.rhs)) &
                  Torus::mask());
  }
}