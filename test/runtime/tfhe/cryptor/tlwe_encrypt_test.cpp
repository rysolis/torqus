#include <gtest/gtest.h>

#include <optional>
#include <random>

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace tlwe_encrypt_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe>
struct ParameterSet {
  using lwe_params = Lwe;
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
  using Lwe = typename Ctx::context::lwe_params;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  Executor<Cryptor<Lwe>, Tracking> exe_;

  void SetUp() override {
    SecretHolder<Lwe> kr(eng_);
    Cryptor<Lwe> cryptor(kr.secret_ptr(), eng_);
    exe_ = Executor<Cryptor<Lwe>, Tracking>(cryptor);
  }
};

template <typename Ctx>
class TlweEncryptionTest : public TlweEncryptionFixture<Ctx> {
 protected:
  using Base = TlweEncryptionFixture<Ctx>;

  using typename Base::Lwe;
  using typename Base::Torus;

  static constexpr uint32_t n = Base::n;

  Torus pt_;

  void SetUp() override {
    Base::SetUp();
    pt_ = Torus(10);
  }
};

TYPED_TEST_SUITE(TlweEncryptionTest, tlwe_encrypt_test::TestContexts);

TYPED_TEST(TlweEncryptionTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;

  using Torus = typename Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  // ==================================
  // Reference
  // ==================================
  Torus ref_pt = this->pt_;

  // ==================================
  // TEST LOGIC
  // ==================================
  TLWE<Torus, n> res_ct = this->exe_.encrypt(this->pt_);
  Torus res_pt = this->exe_.decrypt(res_ct);

  // ----------------------------------
  Torus err = ref_pt - res_pt;
  double norm = infinity_norm(err);

  std::cout << "\n=== TLWE Encryption Test ===\n";
  if (TypeParam::verbose) {
    std::cout << "expected :  " << ref_pt << "\n";
    std::cout << "decrypted:  " << res_pt << "\n";
  }
  std::cout << "infinity_norm: " << norm << "\n";
  // std::cout << "error_bound  : " << sut.error_bound() << "\n";
  std::cout << "===============================\n\n";

  EXPECT_EQ(norm, 0);
}