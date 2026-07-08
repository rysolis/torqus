#include "tfhe/operation/external_product.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/cryptor/glwe_cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"

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

  TrackedCryptor<trlwe::Cryptor<params>> trlwe_cryptor_;
  TrackedCryptor<trgsw::Cryptor<params>> trgsw_cryptor_;

  void SetUp() override {
    SecretHolder<params> kr(this->eng_);
    trlwe_cryptor_ =
        TrackedCryptor<trlwe::Cryptor<params>>(kr.trlwe_cryptor(this->eng_));
    trgsw_cryptor_ =
        TrackedCryptor<trgsw::Cryptor<params>>(kr.trgsw_cryptor(this->eng_));
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {
 protected:
  using Base = ExternalProductFixture<Ctx>;

  using typename Base::params;
  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  Poly<UInt, N> multiplier_;
  TRGSW<Torus, N, l> encrypted_multiplier_;
  Poly<Torus, N> plaintext_;
  TRLWE<Torus, N> encrypted_plaintext_;

  void SetUp() override {
    Base::SetUp();

    multiplier_[0] = UInt(1);

    encrypted_multiplier_ =
        this->trgsw_cryptor_.template encrypt<Torus>(multiplier_);

    std::uniform_int_distribution<typename Torus::raw_value_type> dist(
        Torus::raw_min(), Torus::raw_max());

    plaintext_ =
        Poly<Torus, N>([&]() { return static_cast<Torus>(dist(this->eng_)); });

    encrypted_plaintext_ =
        this->trlwe_cryptor_.template encrypt<Torus>(plaintext_);
  }
};

TYPED_TEST_SUITE(ExternalProductCorrectnessTest,
                 external_product_test::TestContexts);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  // ==================================
  Poly<Torus, N> expected =
      negacyclic_convolution(this->multiplier_, this->plaintext_);
  // ----------------------------------
  TRLWE<Torus, N> sut = TrackedEvaluator<ExternalProduct<params>>::exec(
      this->encrypted_multiplier_, this->encrypted_plaintext_);
  Poly<Torus, N> decrypted = this->trlwe_cryptor_.template decrypt<Torus>(sut);
  // ==================================

  Poly<Torus, N> err = decrypted - expected;
  double norm = infinity_norm(err);

  std::cout << "\n=== External Product Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "plaintext" << ": " << this->plaintext_
              << "\n";
    std::cout << std::setw(14) << "multiplier" << ": " << this->multiplier_
              << "\n";

    std::cout << std::setw(14) << "decrypted" << ": " << decrypted << "\n";
    std::cout << std::setw(14) << "expected" << ": " << expected << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
  std::cout << std::setw(14) << "error_bound" << ": "
            << get_noise_tracker_if()->get(sut) << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(sut));
}
