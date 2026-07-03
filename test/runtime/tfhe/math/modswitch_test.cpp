#include "tfhe/math/modswitch.hpp"
#include <gtest/gtest.h>

#include <cstdint>

#include "primitive/modint.hpp"

namespace modswitch_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <uint32_t N_>
struct ParameterSet {
  static constexpr uint32_t N = N_;
};

using Ctx1 = ParameterSet<4>;
using Ctx2 = ParameterSet<1024>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>>;

}  // namespace modswitch_test

template <typename Ctx>
class ModswitchTest : public ::testing::Test {};

TYPED_TEST_SUITE(ModswitchTest, modswitch_test::TestContexts);

TYPED_TEST(ModswitchTest, Correctness) {
  constexpr uint32_t N = TypeParam::context::N;
  constexpr uint32_t M = 2 * N;

  ModInt<N> a(0);
  ModInt<M> m = mod_switch<M>(a);

  EXPECT_EQ(0, m.value());
}