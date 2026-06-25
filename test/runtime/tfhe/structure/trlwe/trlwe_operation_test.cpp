#include <gtest/gtest.h>

#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace trlwe_operation_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<glwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Ctx2 = ParameterSet<glwe_params<trlwe_core_params<ModTorus<32>, 32>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>>;

}  // namespace trlwe_operation_test

template <typename Ctx>
class TrlweOperationFixture : public ::testing::Test {
 protected:
  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  static inline std::shared_ptr<const Poly<UInt, N>> secret_;
  std::unique_ptr<trlwe::Cryptor<params>> cryptor_;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  void SetUp() override {
    this->eng_.seed(std::random_device{}());
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->secret_ = std::make_shared<const Poly<UInt, N>>(
        [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->cryptor_ =
        std::make_unique<trlwe::Cryptor<params>>(this->secret_, this->eng_);
  }
};

TYPED_TEST_SUITE(TrlweOperationFixture, trlwe_operation_test::TestContexts);

TYPED_TEST(TrlweOperationFixture, AdditionCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  Poly<Torus, N> lhs, rhs;
  lhs[0] = Torus(1);
  rhs[0] = Torus(2);

  Poly<Torus, N> expected = lhs + rhs;

  TRLWE<Torus, N> encrypted_lhs = this->cryptor_->template encrypt<Torus>(lhs);
  TRLWE<Torus, N> encrypted_rhs = this->cryptor_->template encrypt<Torus>(rhs);

  TRLWE<Torus, N> encrypted_add = encrypted_lhs + encrypted_rhs;

  Poly<Torus, N> decrypted =
      this->cryptor_->template decrypt<Torus>(encrypted_add);

  double norm = infinity_norm(decrypted - expected);

  EXPECT_LE(norm, 0.1);
}