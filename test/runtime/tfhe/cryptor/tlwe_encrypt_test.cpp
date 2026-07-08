#include <gtest/gtest.h>

#include <random>

#include "tfhe/cryptor/lwe_cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace tlwe_encrypt_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 4>>>;
using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 600>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace tlwe_encrypt_test

template <typename Ctx>
class TlweEncryptionFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t n = params::n;

  std::unique_ptr<tlwe::Cryptor<params>> cryptor_;

  void SetUp() override {
    SecretHolder<params> kr(this->eng_);
    this->cryptor_ = std::move(kr.tlwe_cryptor(this->eng_));
  }
};

template <typename Ctx>
class TlweEncryptionTest : public TlweEncryptionFixture<Ctx> {
 protected:
  using Base = TlweEncryptionFixture<Ctx>;

  using typename Base::params;
  using typename Base::Torus;

  static constexpr uint32_t n = Base::n;

  Torus plaintext_;

  void SetUp() override {
    Base::SetUp();
    plaintext_ = Torus(10);
  }
};

TYPED_TEST_SUITE(TlweEncryptionTest, tlwe_encrypt_test::TestContexts);

TYPED_TEST(TlweEncryptionTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t n = params::n;

  TLWE<Torus, n> sut =
      this->cryptor_->template encrypt<Torus>(this->plaintext_);
  Torus decrypted = this->cryptor_->template decrypt<Torus>(sut);

  Torus err = decrypted - this->plaintext_;
  double norm = infinity_norm(err);

  std::cout << "\n=== TLWE Encryption Test ===\n";
  if (TypeParam::verbose) {
    std::cout << "expected :  " << this->plaintext_ << "\n";
    std::cout << "decrypted:  " << decrypted << "\n";
  }
  std::cout << "infinity_norm: " << norm << "\n";
  // std::cout << "error_bound  : " << sut.error_bound() << "\n";
  std::cout << "===============================\n\n";

  EXPECT_EQ(norm, 0);
}