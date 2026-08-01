#include <gtest/gtest.h>

#include "primitive/modint.hpp"

namespace modint_arithmetic_test {
struct Ctx1 {
  using modint_type = ModInt<7>;
};
struct Ctx2 {
  using modint_type = ModInt<12289>;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2>;
}  // namespace modint_arithmetic_test

template <typename Ctx>
class ModIntArithmeticTest : public ::testing::Test {
 protected:
  using Zp = typename Ctx::modint_type;

  struct TestCase {
    Zp lhs;
    Zp rhs;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {
        {Zp(1), Zp(2)},
        {Zp(Zp::raw_max()), Zp(1)},
        {Zp(0), Zp(Zp::raw_max())},
    };
  }
};

TYPED_TEST_SUITE(ModIntArithmeticTest, modint_arithmetic_test::TestContexts);

TYPED_TEST(ModIntArithmeticTest, Addition) {
  using Zp = typename TypeParam::modint_type;
  for (const auto& tc : this->cases()) {
    Zp lhs = tc.lhs;
    Zp rhs = tc.rhs;

    Zp res = lhs + rhs;

    Zp ref = [&lhs, &rhs]() {
      return Zp((static_cast<typename Zp::raw_value_type>(lhs) +
                 static_cast<typename Zp::raw_value_type>(rhs)) %
                Zp::MOD);
    }();

    EXPECT_EQ(static_cast<typename Zp::raw_value_type>(ref),
              static_cast<typename Zp::raw_value_type>(res));
  }
}

TYPED_TEST(ModIntArithmeticTest, Subtraction) {
  using Zp = typename TypeParam::modint_type;
  for (const auto& tc : this->cases()) {
    Zp lhs = tc.lhs;
    Zp rhs = tc.rhs;

    Zp res = lhs - rhs;

    Zp ref = [&lhs, &rhs]() {
      if (lhs.value() < rhs.value()) {
        return Zp(static_cast<typename Zp::raw_value_type>(lhs) + Zp::MOD -
                  static_cast<typename Zp::raw_value_type>(rhs));
      } else {
        return Zp(static_cast<typename Zp::raw_value_type>(lhs) -
                  static_cast<typename Zp::raw_value_type>(rhs));
      }
    }();

    EXPECT_EQ(static_cast<typename Zp::raw_value_type>(ref),
              static_cast<typename Zp::raw_value_type>(res));
  }
}
