#include <gtest/gtest.h>

#include <optional>
#include <random>

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace tlwe_encrypt_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe>
struct ParameterSet {
  using lwe_params = Lwe;
};

using Context1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 4>>>;
using Context2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 600>>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2>>;

}  // namespace tlwe_encrypt_test

template <typename Context>
class TlweEncryptionFixture : public ::testing::Test {
 protected:
  using Lwe = typename Context::lwe_params;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  Runtime<Cryptor<Lwe>, Tracking> lwe_runtime_;

  void SetUp() override {
    SecretHolder<Lwe> kr(eng_);
    lwe_runtime_ = Runtime<Cryptor<Lwe>, Tracking>(kr.secret_ptr(), eng_);
  }
};

template <typename Config>
class TlweEncryptionTest
    : public TlweEncryptionFixture<typename Config::context> {
 protected:
  using Base = TlweEncryptionFixture<typename Config::context>;

  using Torus = typename Base::Torus;

  struct TestCase {
    Torus pt;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.pt = Torus(10u)},
            {.pt = Torus(Torus::raw_max())},
            {.pt = random_value<Torus>(this->eng_)}};
  }
};

TYPED_TEST_SUITE(TlweEncryptionTest, tlwe_encrypt_test::TestContexts);

TYPED_TEST(TlweEncryptionTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;

  using Torus = typename Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Torus pt = tc.pt;

    // ==================================
    // Act
    // ==================================
    TLWE<Torus, n> res_ct = this->lwe_runtime_.encrypt(pt);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Torus ref = pt;

    // compute actual result
    Torus res = this->lwe_runtime_.decrypt(res_ct);

    Torus err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n=== TLWE Encryption Test ===\n";
    if (TypeParam::verbose) {
      std::cout << "expected :  " << ref << "\n";
      std::cout << "decrypted:  " << res << "\n";
    }
    std::cout << "infinity_norm: " << norm << "\n";
    // std::cout << "error_bound  : " << sut.error_bound() << "\n";
    std::cout << "===============================\n\n";

    EXPECT_EQ(norm, 0);
  }
}