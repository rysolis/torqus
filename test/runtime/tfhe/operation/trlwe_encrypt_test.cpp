#include <gtest/gtest.h>

#include <random>

#include "algebra/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace trlwe_encrypt_test {

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
using Ctx2 = ParameterSet<glwe_params<trlwe_core_params<ModTorus<8>, 32>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>>;

}  // namespace trlwe_encrypt_test

template <typename Ctx>
class TrlweEncryptionTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  static inline std::shared_ptr<const Poly<UInt, N>> secret_;
  std::unique_ptr<trlwe::Cryptor<params>> cryptor_;

  static inline Poly<Torus, N> plaintext_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->secret_ = std::make_shared<const Poly<UInt, N>>(
        [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->cryptor_ =
        std::make_unique<trlwe::Cryptor<params>>(this->secret_, this->eng_);

    this->plaintext_ =
        Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<Torus>(dist(eng));
        });
  }
};

TYPED_TEST_SUITE(TrlweEncryptionTest, trlwe_encrypt_test::TestContexts);

TYPED_TEST(TrlweEncryptionTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  TRLWE<Torus, N> sut =
      this->cryptor_->template encrypt<Torus>(this->plaintext_);

  Poly<Torus, N> decrypted = this->cryptor_->template decrypt<Torus>(sut);

  Poly<Torus, N> err = decrypted - this->plaintext_;
  double norm = infinity_norm(err);

  std::cout << "\n=== TRLWE Encryption Test ===\n";
  std::cout << "expected : " << this->plaintext_ << "\n";
  std::cout << "decrypted: " << decrypted << "\n";
  std::cout << "infinity_norm    : " << norm << "\n";
  std::cout << "error_bound      : " << sut.error_bound() << "\n";
  std::cout << "===============================\n\n";

  EXPECT_EQ(this->plaintext_, decrypted);
}