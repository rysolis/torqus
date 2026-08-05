#include "tfhe/operation/key_switch.hpp"
#include <gtest/gtest.h>

#include <bit>
#include <random>

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace key_switch_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename SrcLwe, typename DstLwe, typename Kst>
struct ParameterSet {
  using src_lwe_runtime_params = SrcLwe;
  using dst_lwe_runtime_params = DstLwe;
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

  using SrcLwe = Ctx::src_lwe_runtime_params;
  using DstLwe = Ctx::dst_lwe_runtime_params;
  using Kst = Ctx::kst_params;

  using rTorus = SrcLwe::torus_type;
  static constexpr uint32_t N = SrcLwe::n;

  using Torus = DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;

  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  Runtime<Cryptor<SrcLwe>, Tracking> src_lwe_runtime_;

  Runtime<Cryptor<DstLwe>, Tracking> dst_lwe_runtime_;
  KeySwitchKey<Torus, n, t, N> KSK_;

  void SetUp() override {
    SecretHolder<SrcLwe> src_kr(eng_);
    src_lwe_runtime_ =
        Runtime<Cryptor<SrcLwe>, Tracking>(src_kr.secret_ptr(), eng_);

    SecretHolder<DstLwe> dst_kr(eng_);
    dst_lwe_runtime_ =
        Runtime<Cryptor<DstLwe>, Tracking>(dst_kr.secret_ptr(), eng_);

    // Prepare Key Switch Key
    KSK_ =
        dst_lwe_runtime_.template generate_key_switch_key<SrcLwe, DstLwe, Kst>(
            src_kr.secret());
  }
};

template <typename Config>
class KeySwitchCorrectnessTest
    : public KeySwitchFixture<typename Config::context> {
 protected:
  using Base = KeySwitchFixture<typename Config::context>;
  using rTorus = Base::rTorus;

  struct TestCase {
    rTorus pt;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.pt = rTorus(0u);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.pt = rTorus(1u);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(KeySwitchCorrectnessTest, key_switch_test::TestContexts);

TYPED_TEST(KeySwitchCorrectnessTest, VerifyCorrectness) {
  using params = TypeParam::context;
  using SrcLwe = typename params::src_lwe_runtime_params;
  using DstLwe = typename params::dst_lwe_runtime_params;
  using Kst = typename params::kst_params;

  using rTorus = SrcLwe::torus_type;
  constexpr uint32_t N = SrcLwe::n;

  using Torus = DstLwe::torus_type;
  constexpr uint32_t n = DstLwe::n;

  for (const auto& tc : this->cases()) {
    // // ==================================
    // // Arrange
    // // ==================================
    rTorus pt = tc.pt;

    // Prepare source TLWE (contains pt)
    TLWE<rTorus, N> tlwe = this->src_lwe_runtime_.encrypt(Torus(0u));
    tlwe.b() = static_cast<rTorus>(tlwe.b()) + pt;

    // // ==================================
    // // Act
    // // ==================================
    TLWE<Torus, n> res_ct =
        Evaluator<KeySwitch<SrcLwe, DstLwe, Kst>, Tracking>::exec(tlwe,
                                                                  this->KSK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Torus ref = Torus(static_cast<rTorus::raw_value_type>(pt));

    // compute actual result
    Torus res = this->dst_lwe_runtime_.decrypt(res_ct);

    Torus error = ref - res;
    double norm = infinity_norm(error);

    std::cout << "\n=== Key Switch Test ===\n";
    std::cout << std::setw(14) << "actual" << ": " << res << "\n";
    std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
    std::cout << std::setw(14) << "error" << ": " << error << "\n";
    std::cout << std::setw(14) << "norm" << ": " << norm << "\n";
    std::cout << std::setw(14) << "error_bound " << ": "
              << get_noise_tracker_if()->get(res_ct) << '\n';
    std::cout << "=========================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}