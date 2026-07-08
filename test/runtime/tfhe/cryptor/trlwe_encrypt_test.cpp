#include <gtest/gtest.h>

#include <random>

#include "algebra/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"

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
class TrlweEncryptionFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  TrackedCryptor<Cryptor<params>> cryptor_;

  void SetUp() override {
    SecretHolder<params> kr(eng_);
    this->cryptor_ = TrackedCryptor<Cryptor<params>>(kr.secret_ptr(), eng_);
  }
};

template <typename Ctx>
class TrlweEncryptionTest : public TrlweEncryptionFixture<Ctx> {
 protected:
  using Base = TrlweEncryptionFixture<Ctx>;

  using typename Base::params;
  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;

  Poly<Torus, N> plaintext_;

  void SetUp() override {
    Base::SetUp();

    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());
    plaintext_ = Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });
  }
};

TYPED_TEST_SUITE(TrlweEncryptionTest, trlwe_encrypt_test::TestContexts);

TYPED_TEST(TrlweEncryptionTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  TRLWE<Torus, N> encrypted = this->cryptor_.encrypt(this->plaintext_);
  Poly<Torus, N> decrypted = this->cryptor_.decrypt(encrypted);

  Poly<Torus, N> err = decrypted - this->plaintext_;
  double norm = infinity_norm(err);

  std::cout << "\n=== TRLWE Encryption Test ===\n";
  std::cout << "expected : " << this->plaintext_ << "\n";
  std::cout << "decrypted: " << decrypted << "\n";
  std::cout << "infinity_norm    : " << norm << "\n";
  std::cout << "error_bound      : " << get_noise_tracker_if()->get(encrypted)
            << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(encrypted));
}