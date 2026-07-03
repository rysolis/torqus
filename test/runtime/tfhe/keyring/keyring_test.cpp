#include <gtest/gtest.h>

#include <random>

#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

#include "tfhe/keyring/keyring.hpp"
#include "tfhe/params.hpp"

namespace keyring_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename LWE, typename GLWE>
struct ParameterSet {
  using lwe_params = LWE;
  using glwe_params = GLWE;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 32>>,
                          glwe_params<trlwe_core_params<ModTorus<16>, 32>>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 1024>>,
                          glwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;
}  // namespace keyring_test

template <typename T>
class KeyRingTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using lwe_params = T::context::lwe_params;
  using glwe_params = T::context::glwe_params;
  void SetUp() override {
    // nothing to do
  }
};

TYPED_TEST_SUITE(KeyRingTest, keyring_test::TestContexts);

TYPED_TEST(KeyRingTest, ConvertSecret) {
  using lwe_params = typename TypeParam::context::lwe_params;
  using glwe_params = typename TypeParam::context::glwe_params;

  KeyRing<glwe_params> glwe_kr(this->eng_);
  KeyRing<lwe_params> lwe_kr(glwe_kr.secret().begin(), glwe_kr.secret().end());

  std::cout << "\n=== KeyRing Test ===\n";
  if (TypeParam::verbose) {
    std::cout << "GLWE Secret: " << glwe_kr.secret() << "\n";
    std::cout << "LWE Secret:  " << lwe_kr.secret() << "\n";
  }
  for (size_t i = 0; i < lwe_params::n; ++i) {
    EXPECT_EQ(lwe_kr.secret()[i], glwe_kr.secret()[i]);
  }
}