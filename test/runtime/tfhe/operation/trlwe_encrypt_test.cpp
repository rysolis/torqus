#include <gtest/gtest.h>

#include <random>

#include "tfhe/cryptor/cryptor.hpp"

namespace trlwe_encrypt_test {

struct Ctx1 {
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
};

struct Ctx2 {
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2>;

}  // namespace trlwe_encrypt_test

template <typename Ctx>
class TrlweEncryptionTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Torus = typename Ctx::Torus;

  static inline std::shared_ptr<const Poly<UInt>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> cryptor_;

  static inline Poly<Torus> plaintext_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->secret_ = std::make_shared<const Poly<UInt>>(
        Ctx::N, [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->cryptor_ =
        std::make_unique<trlwe::Cryptor<Ctx>>(this->secret_, this->eng_);

    this->plaintext_ =
        Poly<Torus>(Ctx::N, [&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<Torus>(dist(eng));
        });
  }
};

TYPED_TEST_SUITE(TrlweEncryptionTest, trlwe_encrypt_test::TestContexts);

TYPED_TEST(TrlweEncryptionTest, VerifyCorrectness) {
  using Torus = typename TypeParam::Torus;

  TRLWE<Torus> ciphertext =
      this->cryptor_->template encrypt<Torus>(this->plaintext_);

  Poly<Torus> decrypted = this->cryptor_->template decrypt<Torus>(ciphertext);

  std::cout << "\n=== TRLWE Encryption Test ===\n";
  std::cout << "expected : " << this->plaintext_ << "\n";
  std::cout << "decrypted: " << decrypted << "\n";
  std::cout << "diff      : " << decrypted - this->plaintext_ << "\n";
  std::cout << "===============================\n\n";

  EXPECT_EQ(this->plaintext_, decrypted);
}