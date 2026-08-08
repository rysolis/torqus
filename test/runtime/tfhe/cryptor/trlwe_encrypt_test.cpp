#include <gtest/gtest.h>

#include <optional>
#include <random>

#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace trlwe_encrypt_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe>
struct ParameterSet {
  using rlwe_params = Rlwe;
};

using Context1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Context2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<8>, 32>>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace trlwe_encrypt_test

template <typename Context>
class TrlweEncryptionFixture : public ::testing::Test {
 protected:
  using Rlwe = typename Context::rlwe_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  Runtime<Cryptor<Rlwe>, Tracking> rlwe_runtime_;

  void SetUp() override {
    SecretHolder<Rlwe> kr(eng_);
    rlwe_runtime_ = Runtime<Cryptor<Rlwe>, Tracking>(kr.secret_ptr(), eng_);
  }
};

template <typename Config>
class TrlweEncryptionTest
    : public TrlweEncryptionFixture<typename Config::context> {
 protected:
  using Base = TrlweEncryptionFixture<typename Config::context>;

  using typename Base::Torus;
  static constexpr uint32_t N = Base::N;

  struct TestCase {
    Poly<Torus, N> pt;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.pt = randomize<Poly<Torus, N>>(this->eng_)}};
  }
};

TYPED_TEST_SUITE(TrlweEncryptionTest, trlwe_encrypt_test::TestContexts);

TYPED_TEST(TrlweEncryptionTest, VerifyCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Poly<Torus, N> pt = tc.pt;

    // ==================================
    // Act
    // ==================================
    TRLWE<Torus, N> res_ct = this->rlwe_runtime_.encrypt(pt);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<Torus, N> ref = tc.pt;

    // compute actual result
    Poly<Torus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    Poly<Torus, N> err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n=== TRLWE Encryption Test ===\n";
    if (TypeParam::verbose) {
      std::cout << "expected :  " << ref << "\n";
      std::cout << "decrypted:  " << res << "\n";
    }
    std::cout << "infinity_norm    : " << norm << "\n";
    std::cout << "error_bound      : " << get_noise_tracker_if()->get(res_ct)
              << "\n";
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}