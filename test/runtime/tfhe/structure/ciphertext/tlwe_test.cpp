#include "tfhe/structure/ciphertext/tlwe.hpp"
#include <gtest/gtest.h>

#include "primitive/torus.hpp"

#include "algebra/poly.hpp"

#include "tfhe/params.hpp"

namespace trlwe_basic_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 10>>>;
using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 100>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>>;

}  // namespace trlwe_basic_test

template <typename Ctx>
class TlweBasicTest : public ::testing::Test {};

TYPED_TEST_SUITE(TlweBasicTest, trlwe_basic_test::TestContexts);

TYPED_TEST(TlweBasicTest, Constructor) {
  using params = TypeParam::context::params;

  using Torus = params::torus_type;
  constexpr uint32_t n = params::n;

  TLWE<Torus, n> tlwe;

  for (size_t i = 0; i < n; ++i) {
    EXPECT_EQ(Torus(0), tlwe.a()[i]);
  }
  EXPECT_EQ(Torus(0), tlwe.b());
}
