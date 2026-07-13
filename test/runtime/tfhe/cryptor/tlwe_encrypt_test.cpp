#include <gtest/gtest.h>

#include <optional>
#include <random>

#include "tfhe/cryptor.hpp"
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

  std::optional<Cryptor<params>> cryptor_;

  void SetUp() override {
    SecretHolder<params> kr(eng_);
    cryptor_ = std::move(Cryptor<params>(kr.secret_ptr(), eng_));
  }
};

template <typename Ctx>
class TlweEncryptionTest : public TlweEncryptionFixture<Ctx> {
 protected:
  using Base = TlweEncryptionFixture<Ctx>;

  using typename Base::params;
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
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t n = params::n;

  // ==================================
  // Reference
  // ==================================
  Torus ref_pt = this->pt_;

  // ==================================
  // TEST LOGIC
  // ==================================
  TLWE<Torus, n> res_ct = this->cryptor_->encrypt(this->pt_);
  Torus res_pt = this->cryptor_->decrypt(res_ct);

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