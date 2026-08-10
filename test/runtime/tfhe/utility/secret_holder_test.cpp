#include <gtest/gtest.h>

#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

#include "tfhe/params.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace keyring_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 32>>,
                          rlwe_params<trlwe_core_params<ModTorus<16>, 32>>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 1024>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;
}  // namespace keyring_test

template <typename T>
class SecretHolderTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using lwe_params = T::context::lwe_params;
  using rlwe_params = T::context::rlwe_params;
  void SetUp() override {
    // nothing to do
  }
};

TYPED_TEST_SUITE(SecretHolderTest, keyring_test::TestContexts);

TYPED_TEST(SecretHolderTest, ConvertSecret) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;

  SecretHolder<Rlwe::N> rlwe(this->eng_);
  SecretHolder<Lwe::n> lwe(rlwe);

  std::cout << "\n=== SecretHolder Test ===\n";
  if (TypeParam::verbose) {
    std::cout << "GLWE Secret: " << rlwe.get() << "\n";
    std::cout << "LWE Secret:  " << lwe.get() << "\n";
  }
  for (size_t i = 0; i < Lwe::n; ++i) {
    EXPECT_EQ(lwe.get()[i], rlwe.get()[i]);
  }
}