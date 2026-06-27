#include "tfhe/operation/external_product.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace external_product_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<16>, 4>, gadget_params<4, 3>>>;
using Ctx2 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<16>, 8>, gadget_params<8, 3>>>;
using Ctx3 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<32>, 1024>, gadget_params<256, 2>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>,
                                      TestConfig<Ctx3, false>>;

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  static inline std::shared_ptr<const Poly<UInt, N>> secret_;
  std::unique_ptr<trlwe::Cryptor<params>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<params>> trgsw_cryptor_;

  // ============================================================
  // test inputs
  // ============================================================

  Poly<UInt, N> multiplier_;
  TRGSW<Torus, N, l> encrypted_multiplier_;

  Poly<Torus, N> plaintext_;

  ExternalProduct<params> extprod_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->plaintext_ =
        Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<Torus>(dist(eng));
        });

    // secret
    this->secret_ = std::make_shared<const Poly<UInt, N>>(
        [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->trlwe_cryptor_ =
        std::make_unique<trlwe::Cryptor<params>>(this->secret_, this->eng_);

    this->trgsw_cryptor_ =
        std::make_unique<trgsw::Cryptor<params>>(this->secret_, this->eng_);

    this->multiplier_[0] = UInt(1);

    this->encrypted_multiplier_ =
        trgsw_cryptor_->template encrypt<Torus>(this->multiplier_);
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {};

TYPED_TEST_SUITE(ExternalProductCorrectnessTest,
                 external_product_test::TestContexts);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  TRLWE<Torus, N> encrypted =
      this->trlwe_cryptor_->template encrypt<Torus>(this->plaintext_);

  // ==================================
  Poly<Torus, N> expected =
      negacyclic_convolution(this->multiplier_, this->plaintext_);
  // ----------------------------------
  TRLWE<Torus, N> sut = this->extprod_(this->encrypted_multiplier_, encrypted);
  Poly<Torus, N> decrypted = this->trlwe_cryptor_->template decrypt<Torus>(sut);
  // ==================================

  Poly<Torus, N> err = decrypted - expected;
  double norm = infinity_norm(err);

  std::cout << "\n=== External Product Test ===\n";
  if (TypeParam::verbose) {
    std::cout << "secret    : " << *(this->secret_) << "\n";
    std::cout << "plaintext : " << this->plaintext_ << "\n";
    std::cout << "multiplier: " << this->multiplier_ << "\n";

    std::cout << "decrypted : " << decrypted << "\n";
    std::cout << "expected  : " << expected << "\n";
  }
  std::cout << "infinity_norm: " << norm << "\n";
  std::cout << "error_bound  : " << sut.error_bound() << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, sut.error_bound());
}
