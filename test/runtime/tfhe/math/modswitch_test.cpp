#include "tfhe/math/modswitch.hpp"
#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include "primitive/modint.hpp"

#include "tfhe/utility/random_generator.hpp"

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

template <uint32_t Src_, uint32_t Dst_>
struct RoundingParameterSet {
  static constexpr uint32_t Src = Src_;
  static constexpr uint32_t Dst = Dst_;
};

using RoundingCtx1 = RoundingParameterSet<1u << 16, 2 * 4>;
using RoundingCtx2 = RoundingParameterSet<0u, 2 * 1024>;

using RoundingTestContexts =
    ::testing::Types<TestConfig<RoundingCtx1>, TestConfig<RoundingCtx2>>;

}  // namespace modswitch_test

template <typename Ctx>
class ModswitchTrivialTest : public ::testing::Test {};

TYPED_TEST_SUITE(ModswitchTrivialTest, modswitch_test::TestContexts);

TYPED_TEST(ModswitchTrivialTest, Correctness) {
  constexpr uint32_t N = TypeParam::context::N;
  constexpr uint32_t M = 2 * N;

  ModInt<N> a(0);
  ModInt<M> m = mod_switch<M>(a);

  EXPECT_EQ(0, m.value());
}

// mod_switch<Dst> is a round-to-nearest, so its error is bounded by
// eps = 1/(2*Dst). Correctness above only covers Src=N/Dst=2*N, which is
// lossless (a left shift) -- this covers the real Src >> Dst direction
// gate_bootstrap.hpp actually uses.
template <typename Ctx>
class ModswitchRoundingTest : public ::testing::Test {};

TYPED_TEST_SUITE(ModswitchRoundingTest, modswitch_test::RoundingTestContexts);

TYPED_TEST(ModswitchRoundingTest, RoundingErrorBound) {
  constexpr uint32_t Src = TypeParam::context::Src;
  constexpr uint32_t Dst = TypeParam::context::Dst;
  using Raw = typename ModInt<Src>::raw_value_type;

  // Src == 0 stands in for a full-width modulus (2^32 or 2^64, depending
  // on TORQUS_TORUS_BITS -- see ModInt's own "mod == 0 means natural
  // wraparound" convention), so its span tracks Raw's own width rather
  // than a hardcoded 32-bit span.
  constexpr double src_span =
      Src != 0 ? double(Src)
      : std::numeric_limits<Raw>::digits >= 64
          ? 18446744073709551616.0  // 2^64, too wide to shift into a Raw
          : double(uint64_t{1} << std::numeric_limits<Raw>::digits);
  constexpr double eps = 1.0 / (2.0 * double(Dst));

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng{0};
  std::uniform_int_distribution<Raw> dist(0u, ModInt<Src>::raw_max());

  std::vector<Raw> raws = {0u, 1u, ModInt<Src>::raw_max() / 2,
                           ModInt<Src>::raw_max()};
  for (int i = 0; i < 8; ++i) raws.push_back(dist(eng));

  for (Raw raw : raws) {
    ModInt<Src> t(raw);
    ModInt<Dst> switched = mod_switch<Dst>(t);

    double original = double(t.value()) / src_span;
    double rounded = double(switched.value()) / double(Dst);

    double diff = std::fabs(original - rounded);
    double torus_dist = std::min(diff, 1.0 - diff);

    EXPECT_LE(torus_dist, eps) << "raw=" << raw;
  }
}
