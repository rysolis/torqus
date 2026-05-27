#include "tfhe/operation/external_product.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/structure/trgsw.hpp"

namespace external_product_test {

struct Ctx1 {
  static constexpr bool verbose = true;
  using torus_type = ModTorus;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx2 {
  static constexpr bool verbose = true;
  using torus_type = Torus;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx3 {
  static constexpr bool verbose = true;
  using torus_type = ModTorus;
  static constexpr uint32_t N = 8;
  static constexpr uint32_t B = 8;
  static constexpr uint32_t l = 3;
};

struct Ctx4 {
  static constexpr bool verbose = true;
  using torus_type = ModTorus;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using torus_type = typename Ctx::torus_type;

  static inline std::shared_ptr<const Poly<UInt>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<Ctx>> trgsw_cryptor_;

  bool verbose = Ctx::verbose;

  // ============================================================
  // shared test inputs
  // ============================================================

  static inline Poly<UInt> multiplier_;
  static inline TRGSW<torus_type> encrypted_multiplier_;

  static inline Poly<torus_type> plaintext_;

  static inline ExternalProduct<Ctx> extprod_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    auto torus_dist = [&] {
      if constexpr (std::same_as<torus_type, ModTorus>) {
        return std::uniform_int_distribution<ModTorus::raw_value_type>(
            0, Torus::Q - 1);
      } else if constexpr (std::same_as<torus_type, Torus>) {
        return std::uniform_real_distribution<Torus::raw_value_type>(0.0, 1.0);
      } else {
        static_assert(false, "unsupported torus_type");
      }
    }();

    this->plaintext_ =
        Poly<torus_type>(Ctx::N, [&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<torus_type>(dist(eng));
        });

    // secret
    this->secret_ = std::make_shared<const Poly<UInt>>(
        Ctx::N, [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->trlwe_cryptor_ = std::make_unique<trlwe::Cryptor<Ctx>>(
        this->secret_, this->eng_, torus_dist);

    this->trgsw_cryptor_ = std::make_unique<trgsw::Cryptor<Ctx>>(
        this->secret_, this->eng_, torus_dist);

    this->multiplier_ = Poly<UInt>(Ctx::N);
    this->multiplier_[0] = UInt(1);

    this->encrypted_multiplier_ =
        trgsw_cryptor_->template encrypt<torus_type>(this->multiplier_);
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {};

using TestContextsCorrectness =
    ::testing::Types<external_product_test::Ctx1, external_product_test::Ctx2,
                     external_product_test::Ctx3>;

TYPED_TEST_SUITE(ExternalProductCorrectnessTest, TestContextsCorrectness);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using Ctx = TypeParam;
  using torus_type = Ctx::torus_type;

  TRLWE<torus_type> encrypted =
      this->trlwe_cryptor_->template encrypt<torus_type>(this->plaintext_);

  // ==================================
  Poly<torus_type> expected = this->multiplier_ * this->plaintext_;
  // ----------------------------------
  Poly<torus_type> decrypted = [&] {
    TRLWE<torus_type> hom_mul = this->extprod_(this->encrypted_multiplier_,
                                               convert_to<ModTorus>(encrypted));

    return this->trlwe_cryptor_->template decrypt<torus_type>(hom_mul);
  }();
  // ==================================

  double norm = infinity_norm(decrypted - expected);

  if (this->verbose) {
    std::cout << "\n=== External Product Test ===\n";
    std::cout << "secret    : " << *(this->secret_) << "\n";
    std::cout << "plaintext : " << this->plaintext_ << "\n";
    std::cout << "multiplier: " << this->multiplier_ << "\n\n";

    std::cout << "decrypted : " << decrypted << "\n";
    std::cout << "expected  : " << expected << "\n";
    std::cout << "infinity_norm: " << norm << "\n";
    std::cout << "===============================\n\n";
  }

  EXPECT_LE(norm, ExternalProduct<Ctx>::threshold);
}

template <typename Ctx>
class ExternalProductConsistencyTest : public ExternalProductFixture<Ctx> {};

using TestContextsConsistency =
    ::testing::Types<external_product_test::Ctx3, external_product_test::Ctx4>;

TYPED_TEST_SUITE(ExternalProductConsistencyTest, TestContextsConsistency);

TYPED_TEST(ExternalProductConsistencyTest, VerifyConsistency) {
  using Ctx = TypeParam;

  TRLWE<ModTorus> encrypted =
      this->trlwe_cryptor_->template encrypt<ModTorus>(this->plaintext_);

  // ==================================
  // Compute refernce by using Torus
  // ==================================
  Poly<Torus> reference = [&] {
    TRLWE<Torus> hom_mul = this->extprod_(
        convert_to<Torus>(this->encrypted_multiplier_), encrypted);

    return this->trlwe_cryptor_->template decrypt<Torus>(hom_mul);
  }();
  // ----------------------------------
  // In parctice, use ModTorus
  // ----------------------------------
  Poly<ModTorus> decrypted = [&] {
    TRLWE<ModTorus> hom_mul =
        this->extprod_(this->encrypted_multiplier_, encrypted);

    return this->trlwe_cryptor_->template decrypt<ModTorus>(hom_mul);
  }();
  // ==================================

  double norm = infinity_norm(convert_to<Torus>(decrypted) - reference);

  std::cout << "\n=== External Product Test ===\n";
  std::cout << "secret    : " << *(this->secret_) << "\n";
  std::cout << "plaintext : " << this->plaintext_ << "\n";
  std::cout << "multiplier: " << this->multiplier_ << "\n\n";

  std::cout << "decrypted : " << decrypted << "\n";
  std::cout << "reference : " << reference << "\n";
  std::cout << "infinity_norm: " << norm << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, ExternalProduct<Ctx>::threshold);
}
