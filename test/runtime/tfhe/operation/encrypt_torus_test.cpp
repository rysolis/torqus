#include <gtest/gtest.h>

#include <random>

#include "tfhe/cryptor/cryptor.hpp"

namespace encrypt_torus_test {

struct Ctx1 {
  using torus_type = Torus;
  static constexpr uint32_t N = 4;
};

struct Ctx2 {
  using torus_type = ModTorus;
  static constexpr uint32_t N = 4;
};

}  // namespace encrypt_torus_test

using TestContexts = ::testing::Types<encrypt_torus_test::Ctx1>;

template <typename Ctx>
class TrlweEncryptionTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using torus_type = Ctx::torus_type;

  static inline std::shared_ptr<const Poly<UInt>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> cryptor_;

  static inline Poly<torus_type> plaintext_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    auto torus_dist = [&] {
      if constexpr (std::same_as<torus_type, ModTorus>) {
        return std::uniform_int_distribution<ModTorus::raw_value_type>(
            0, Torus::Q - 1);
      }
      if constexpr (std::same_as<torus_type, Torus>) {
        return std::uniform_real_distribution<Torus::raw_value_type>(0.0, 1.0);
      } else {
        static_assert(false, "unsupported torus_type");
      }
    }();

    this->secret_ = std::make_shared<const Poly<UInt>>(
        Ctx::N, [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->cryptor_ = std::make_unique<trlwe::Cryptor<Ctx>>(
        this->secret_, this->eng_, torus_dist);

    this->plaintext_ =
        Poly<torus_type>(Ctx::N, [&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<torus_type>(dist(eng));
        });
  }
};

TYPED_TEST_SUITE(TrlweEncryptionTest, TestContexts);

TYPED_TEST(TrlweEncryptionTest, VerifyCorrectness) {
  using torus_type = TypeParam::torus_type;

  TRLWE<torus_type> ciphertext =
      this->cryptor_->template encrypt<torus_type>(this->plaintext_);

  Poly<torus_type> decrypted =
      this->cryptor_->template decrypt<torus_type>(ciphertext);

  std::cout << "\n=== TRLWE Encryption Test ===\n";
  std::cout << "expected : " << this->plaintext_ << "\n";
  std::cout << "decrypted: " << decrypted << "\n";
  std::cout << "diff      : " << decrypted - this->plaintext_ << "\n";
  std::cout << "===============================\n\n";

  EXPECT_EQ(decrypted, this->plaintext_);
}