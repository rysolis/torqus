#include <gtest/gtest.h>

#include <optional>
#include <random>

#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace trlwe_encrypt_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe>
struct ParameterSet {
  using rlwe_params = Rlwe;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<8>, 32>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>>;

}  // namespace trlwe_encrypt_test

template <typename Ctx>
class TrlweEncryptionFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using Rlwe = typename Ctx::context::rlwe_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  Executor<Cryptor<Rlwe>, Tracking> exe_;

  void SetUp() override {
    SecretHolder<Rlwe> kr(eng_);
    Cryptor<Rlwe> cryptor(kr.secret_ptr(), eng_);
    exe_ = Executor<Cryptor<Rlwe>, Tracking>(cryptor);
  }
};

template <typename Ctx>
class TrlweEncryptionTest : public TrlweEncryptionFixture<Ctx> {
 protected:
  using Base = TrlweEncryptionFixture<Ctx>;

  using typename Base::Torus;
  static constexpr uint32_t N = Base::N;

  Poly<Torus, N> pt_;

  void SetUp() override {
    Base::SetUp();
    randomize(pt_, this->eng_);
  }
};

TYPED_TEST_SUITE(TrlweEncryptionTest, trlwe_encrypt_test::TestContexts);

TYPED_TEST(TrlweEncryptionTest, VerifyCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  // ==================================
  // Reference
  // ==================================
  Poly<Torus, N> ref_pt = this->pt_;

  // ==================================
  // TEST LOGIC
  // ==================================
  TRLWE<Torus, N> res_ct = this->exe_.encrypt(this->pt_);
  Poly<Torus, N> res_pt = this->exe_.decrypt(res_ct);

  // ----------------------------------
  Poly<Torus, N> err = ref_pt - res_pt;
  double norm = infinity_norm(err);

  std::cout << "\n=== TRLWE Encryption Test ===\n";
  std::cout << "expected : " << ref_pt << "\n";
  std::cout << "decrypted: " << res_pt << "\n";
  std::cout << "infinity_norm    : " << norm << "\n";
  std::cout << "error_bound      : " << get_noise_tracker_if()->get(res_ct)
            << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}