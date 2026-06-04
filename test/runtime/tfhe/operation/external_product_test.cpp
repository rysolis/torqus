#include "tfhe/operation/external_product.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"
#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/structure/trgsw.hpp"

namespace external_product_test {

struct Ctx1 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx2 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx3 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 8;
  static constexpr uint32_t B = 8;
  static constexpr uint32_t l = 3;
};

struct Ctx4 {
  static constexpr bool verbose = false;
  using Torus = ModTorus<32>;
  static constexpr uint32_t N = 1024;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 7;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3, Ctx4>;

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Torus = typename Ctx::Torus;

  static inline std::shared_ptr<const Poly<UInt>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<Ctx>> trgsw_cryptor_;

  // ============================================================
  // test inputs
  // ============================================================

  Poly<UInt> multiplier_;
  TRGSW<Torus> encrypted_multiplier_;

  Poly<Torus> plaintext_;

  ExternalProduct<Ctx> extprod_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->plaintext_ =
        Poly<Torus>(Ctx::N, [&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<Torus>(dist(eng));
        });

    // secret
    this->secret_ = std::make_shared<const Poly<UInt>>(
        Ctx::N, [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->trlwe_cryptor_ =
        std::make_unique<trlwe::Cryptor<Ctx>>(this->secret_, this->eng_);

    this->trgsw_cryptor_ =
        std::make_unique<trgsw::Cryptor<Ctx>>(this->secret_, this->eng_);

    this->multiplier_ = Poly<UInt>(Ctx::N);
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
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TRLWE<Torus> encrypted =
      this->trlwe_cryptor_->template encrypt<Torus>(this->plaintext_);

  // ==================================
  Poly<Torus> expected = this->multiplier_ * this->plaintext_;
  // ----------------------------------
  TRLWE<Torus> hom_mul = this->extprod_(this->encrypted_multiplier_, encrypted);
  Poly<Torus> decrypted =
      this->trlwe_cryptor_->template decrypt<Torus>(hom_mul);
  // ==================================

  double norm = infinity_norm(decrypted - expected);

  std::cout << "\n=== External Product Test ===\n";
  if (Ctx::verbose) {
    std::cout << "secret    : " << *(this->secret_) << "\n";
    std::cout << "plaintext : " << this->plaintext_ << "\n";
    std::cout << "multiplier: " << this->multiplier_ << "\n\n";

    std::cout << "decrypted : " << decrypted << "\n";
    std::cout << "expected  : " << expected << "\n";
  }
  std::cout << "infinity_norm: " << norm << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, ExternalProduct<Ctx>::threshold);
}
