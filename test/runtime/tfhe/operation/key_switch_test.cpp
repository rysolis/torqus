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
#include "tfhe/utility/random_generator.hpp"

namespace key_switch_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename SrcLwe, typename DstLwe, typename Kst>
struct ParameterSet {
  using src_lwe_runtime_params = SrcLwe;
  using dst_lwe_runtime_params = DstLwe;
  using kst_params = Kst;
};

using Context1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 8>>,
                              lwe_params<tlwe_core_params<ModTorus<16>, 5>>,
                              kst_params<8, 5>>;

using Context2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 1024>>,
                              lwe_params<tlwe_core_params<ModTorus<32>, 200>>,
                              kst_params<4, 12>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace key_switch_test

template <typename Context>
class KeySwitchFixture : public ::testing::Test {
 public:
  using SrcLwe = Context::src_lwe_runtime_params;
  using DstLwe = Context::dst_lwe_runtime_params;
  using Kst = Context::kst_params;

  using rTorus = typename SrcLwe::torus_type;
  static constexpr uint32_t N = SrcLwe::n;

  using Torus = typename DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;

  static constexpr uint32_t t = Kst::t;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{1};

  Runtime<Cryptor<SrcLwe>, Tracking> src_lwe_runtime_;

  Runtime<Cryptor<DstLwe>, Tracking> dst_lwe_runtime_;
  KeySwitchKey<Torus, n, t, N> KSK_;

  void SetUp() override {
    src_lwe_runtime_ = Runtime<Cryptor<SrcLwe>, Tracking>(eng_);
    dst_lwe_runtime_ = Runtime<Cryptor<DstLwe>, Tracking>(eng_);

    // Prepare Key Switch Key
    KSK_ =
        dst_lwe_runtime_.template generate_key_switch_key<SrcLwe, DstLwe, Kst>(
            src_lwe_runtime_.holder().get());
  }
};

template <typename Config>
class KeySwitchCorrectnessTest
    : public KeySwitchFixture<typename Config::context> {
 protected:
  using Base = KeySwitchFixture<typename Config::context>;
  using rTorus = typename Base::rTorus;

  struct TestCase {
    rTorus pt;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.pt = rTorus(0u)}, {.pt = rTorus(1u)}};
  }
};

TYPED_TEST_SUITE(KeySwitchCorrectnessTest, key_switch_test::TestContexts);

TYPED_TEST(KeySwitchCorrectnessTest, VerifyCorrectness) {
  using params = TypeParam::context;
  using SrcLwe = typename params::src_lwe_runtime_params;
  using DstLwe = typename params::dst_lwe_runtime_params;
  using Kst = typename params::kst_params;

  using rTorus = typename SrcLwe::torus_type;
  constexpr uint32_t N = SrcLwe::n;

  using Torus = typename DstLwe::torus_type;
  constexpr uint32_t n = DstLwe::n;

  for (const auto& tc : TestFixture::cases()) {
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
    Torus ref(static_cast<rTorus::raw_value_type>(pt));

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