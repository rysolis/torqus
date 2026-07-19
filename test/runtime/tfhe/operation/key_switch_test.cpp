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

template <typename SrcLwe, typename DstLwe, typename Kst>
struct ParameterSet {
  using src_params = SrcLwe;
  using dst_params = DstLwe;
  using kst_params = Kst;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 8>>,
                          lwe_params<tlwe_core_params<ModTorus<16>, 5>>,
                          kst_params<8, 5>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 1024>>,
                          lwe_params<tlwe_core_params<ModTorus<32>, 200>>,
                          kst_params<4, 12>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace key_switch_test

template <typename Ctx>
class KeySwitchFixture : public ::testing::Test {
 public:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{1};

  using SrcLwe = Ctx::src_params;
  using DstLwe = Ctx::dst_params;
  using Kst = Ctx::kst_params;

  using rTorus = SrcLwe::torus_type;
  static constexpr uint32_t N = SrcLwe::n;

  using Torus = DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;

  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  rTorus pt_ = rTorus(1);
  TrackedCryptor<Cryptor<SrcLwe>> src_cryptor_;
  TrackedCryptor<Cryptor<DstLwe>> dst_cryptor_;

  TLWE<rTorus, N> src_tlwe_;

  KeySwitchKey<Torus, n, t, N> KSK_;

  void SetUp() override {
    SecretHolder<SrcLwe> src_kr(eng_);
    SecretHolder<DstLwe> dst_kr(eng_);

    src_cryptor_ = TrackedCryptor<Cryptor<SrcLwe>>(src_kr.secret_ptr(), eng_);
    dst_cryptor_ = TrackedCryptor<Cryptor<DstLwe>>(dst_kr.secret_ptr(), eng_);

    // prepare source tlwe
    src_tlwe_ = src_cryptor_.encrypt(pt_);

    // Prepare Key Switch Key
    KSK_ = keyswitch_key::generate<SrcLwe, DstLwe, Kst>(dst_cryptor_, src_kr);
  }
};

template <typename Config>
class KeySwitchCorrectnessTest
    : public KeySwitchFixture<typename Config::context> {};

TYPED_TEST_SUITE(KeySwitchCorrectnessTest, key_switch_test::TestContexts);

TYPED_TEST(KeySwitchCorrectnessTest, VefiryCorrectness) {
  using params = TypeParam::context;
  using SrcLwe = typename params::src_params;
  using DstLwe = typename params::dst_params;
  using Kst = typename params::kst_params;

  using rTorus = SrcLwe::torus_type;

  using Torus = DstLwe::torus_type;
  constexpr uint32_t n = DstLwe::n;

  // ==================================
  // Reference
  // ==================================
  Torus expected = Torus(static_cast<rTorus::raw_value_type>(this->pt_));

  // ==================================
  // TEST LOGIC
  // ==================================
  TLWE<Torus, n> encrypted =
      TrackedEvaluator<KeySwitch<SrcLwe, DstLwe, Kst>>::exec(this->src_tlwe_,
                                                             this->KSK_);
  Torus decrypted = this->dst_cryptor_.decrypt(encrypted);

  // ----------------------------------

  Torus error = expected - decrypted;
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