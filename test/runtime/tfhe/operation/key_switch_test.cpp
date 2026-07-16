#include "tfhe/operation/key_switch.hpp"
#include <gtest/gtest.h>

#include <bit>
#include <random>

#include "tfhe/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace key_switch_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename SRC, typename DST>
struct ParameterSet {
  using src_lwe_params = SRC;
  using dst_lwe_params = DST;
};

using Ctx1 = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<16>, 8>, key_switch_params<8, 5>>,
    lwe_params<tlwe_core_params<ModTorus<16>, 5>>>;

using Ctx2 = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<32>, 1024>, key_switch_params<4, 12>>,
    lwe_params<tlwe_core_params<ModTorus<32>, 200>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace key_switch_test

template <typename Ctx>
class KeySwitchFixture : public ::testing::Test {
 public:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{1};

  using src_lwe_params = Ctx::src_lwe_params;
  using dst_lwe_params = Ctx::dst_lwe_params;

  using bTorus = src_lwe_params::torus_type;
  static constexpr uint32_t N = src_lwe_params::n;
  static constexpr uint32_t K = src_lwe_params::K;
  static constexpr uint32_t t = src_lwe_params::t;
  static constexpr uint32_t Kbit = std::bit_width(K - 1);

  using fTorus = dst_lwe_params::torus_type;
  static constexpr uint32_t n = dst_lwe_params::n;

  bTorus pt_ = bTorus(1);
  TrackedCryptor<Cryptor<src_lwe_params>> src_cryptor_;
  TrackedCryptor<Cryptor<dst_lwe_params>> dst_cryptor_;

  TLWE<bTorus, N> src_tlwe_;

  KeySwitchKey<fTorus, n, t, N> KSK_;

  void SetUp() override {
    SecretHolder<src_lwe_params> src_kr(eng_);
    SecretHolder<dst_lwe_params> dst_kr(eng_);

    src_cryptor_ =
        TrackedCryptor<Cryptor<src_lwe_params>>(src_kr.secret_ptr(), eng_);
    dst_cryptor_ =
        TrackedCryptor<Cryptor<dst_lwe_params>>(dst_kr.secret_ptr(), eng_);

    // prepare source tlwe
    src_tlwe_ = src_cryptor_.encrypt(pt_);

    // Prepare Key Switch Key
    for (uint32_t i = 0; i < N; ++i) {
      fTorus s(src_kr.secret_ptr()[i]);
      for (size_t j = 0; j < t; ++j) {
        typename fTorus::raw_value_type tmp =
            static_cast<typename fTorus::raw_value_type>(s)
            << (fTorus::qbit - Kbit * (j + 1));
        KSK_[i][j] = dst_cryptor_.encrypt(fTorus(tmp));
      }
    }
  }
};

template <typename Config>
class KeySwitchCorrectnessTest
    : public KeySwitchFixture<typename Config::context> {};

TYPED_TEST_SUITE(KeySwitchCorrectnessTest, key_switch_test::TestContexts);

TYPED_TEST(KeySwitchCorrectnessTest, VefiryCorrectness) {
  using src_lwe_params = typename TypeParam::context::src_lwe_params;
  using dst_lwe_params = typename TypeParam::context::dst_lwe_params;

  using fTorus = dst_lwe_params::torus_type;
  constexpr uint32_t n = dst_lwe_params::n;

  using bTorus = src_lwe_params::torus_type;

  // ==================================
  fTorus expected = fTorus(static_cast<bTorus::raw_value_type>(this->pt_));
  // ----------------------------------
  TLWE<fTorus, n> encrypted =
      TrackedEvaluator<KeySwitch<dst_lwe_params, src_lwe_params>>::exec(
          this->src_tlwe_, this->KSK_);
  fTorus decrypted = this->dst_cryptor_.decrypt(encrypted);
  // ==================================

  fTorus error = expected - decrypted;
  double norm = infinity_norm(error);

  std::cout << "\n=== Key Switch Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::setw(14) << "decrypted" << ": " << decrypted << "\n";
    std::cout << std::setw(14) << "expected" << ": " << expected << "\n";
  }
  std::cout << std::setw(14) << "error" << ": " << error << "\n";
  std::cout << std::setw(14) << "norm" << ": " << norm << "\n";
  std::cout << std::setw(14) << "errror_bound " << ": "
            << get_noise_tracker_if()->get(encrypted) << '\n';
  std::cout << "=========================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(encrypted));
}