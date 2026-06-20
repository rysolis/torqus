#include "tfhe/utility/modswitch.hpp"

#include <gtest/gtest.h>

#include <cstdint>

#include "primitive/modint.hpp"

namespace modswitch_test {
struct Ctx1 {
  static constexpr uint32_t N = 4;
};

using TestContexts = ::testing::Types<Ctx1>;

}  // namespace modswitch_test

template <typename Ctx>
class ModswitchTest : public ::testing::Test {};

TYPED_TEST_SUITE(ModswitchTest, modswitch_test::TestContexts);

TYPED_TEST(ModswitchTest, Correctness) {
  using Ctx = TypeParam;
  constexpr uint32_t N = Ctx::N;
  constexpr uint32_t M = 2 * Ctx::N;

  ModInt<N> a(0);
  ModInt<M> m = mod_switch<M>(a);

  EXPECT_EQ(0, m.value());
}