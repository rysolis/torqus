#include "tfhe/circuit/binary_expansion.hpp"
#include <gtest/gtest.h>

#include <iomanip>

#include "primitive/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/dial.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace binary_expansion_test {
template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe, typename Decomp, typename Kst>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
  using dcp_params = Decomp;
  using kst_params = Kst;
};

using Context1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 4>>,
                              rlwe_params<trlwe_core_params<ModTorus<16>, 32>>,
                              dcp_params<4, 6>, kst_params<4, 6>>;

// Real noise enabled (see gate_bootstrap_test.cpp's Context2). kst_params's
// base must stay small here -- KeySwitch noise grows as N*t*(K-1) (see
// noise.hpp), so K=256 blew past the decode margin; K=2 (binary) fixes it.
using Context2 = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>,
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>,
    dcp_params<16, 7>, kst_params<2, 11>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;
}  // namespace binary_expansion_test

template <typename Context>
class BinaryExpansionFixture : public ::testing::Test {
 protected:
  using Lwe = Context::lwe_params;
  using Rlwe = Context::rlwe_params;
  using Decomp = Context::dcp_params;
  using Kst = Context::kst_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = Lwe::torus_type;
  using rTorus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe, Tracking> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Tracking> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> BK_;
  KeySwitchKey<Torus, n, t, N> KSK_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>, Tracking>(eng_);
    lwe_runtime_ = Runtime<Lwe, Tracking>(eng_);

    // Prepare Bootstrapkey
    BK_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime_.holder().get());
    KSK_ = lwe_runtime_
               .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                   rlwe_runtime_.holder().get());
  }
};

template <typename Config>
class BinaryExpansionCorrectnessTest
    : public BinaryExpansionFixture<typename Config::context> {
 protected:
  using Base = BinaryExpansionFixture<typename Config::context>;

  using Torus = Base::Torus;

  // BinaryExpansion<4, ...> turns a 2-bit operand into a one-hot 4-output
  // vector -- exactly one output (at index `hot`) decodes to Dial's true
  // slot, the rest to false.
  struct TestCase {
    Vector<Torus, 2> operand;
    uint32_t hot;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.operand = {Dial<4, Torus>(false).value(),
                         Dial<4, Torus>(false).value()},
             .hot = 0},
            {.operand = {Dial<4, Torus>(true).value(),
                         Dial<4, Torus>(false).value()},
             .hot = 1},
            {.operand = {Dial<4, Torus>(false).value(),
                         Dial<4, Torus>(true).value()},
             .hot = 2},
            {.operand = {Dial<4, Torus>(true).value(),
                         Dial<4, Torus>(true).value()},
             .hot = 3}};
  }
};

TYPED_TEST_SUITE(BinaryExpansionCorrectnessTest,
                 binary_expansion_test::TestContexts);

TYPED_TEST(BinaryExpansionCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;
  using Kst = typename TypeParam::context::kst_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Vector<Torus, 2> operand = tc.operand;

    Vector<TLWE<Torus, n>, 2> operand_ct;
    operand_ct[0] = this->lwe_runtime_.encrypt(static_cast<Torus>(operand[0]));
    operand_ct[1] = this->lwe_runtime_.encrypt(static_cast<Torus>(operand[1]));

    // ==================================
    // Act
    // ==================================
    Vector<TLWE<rTorus, N>, 4> res_ct =
        tfhe::circuit::BinaryExpansion<4, Lwe, Rlwe, Decomp, Kst>::exec_impl(
            operand_ct, this->KSK_, this->BK_);

    // ==================================
    // Assert
    // ==================================
    std::cout << "\n========================================\n";
    std::cout << "         BinaryExpansion Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "operand" << ": " << operand << "\n";
    std::cout << std::setw(14) << "hot index" << ": " << tc.hot << "\n";

    // Dial::index() already tolerates noise up to margin() on its own, so
    // there is no need to separately compute an infinity norm and compare
    // it against that margin by hand -- decoding each output directly says
    // whether it landed on the right slot.
    for (uint32_t i = 0; i < 4; ++i) {
      rTorus decrypted = this->rlwe_runtime_.decrypt(res_ct[i]);
      uint32_t index = Dial<4, rTorus>(decrypted).index();
      uint32_t expected = (i == tc.hot) ? 1u : 0u;
      EXPECT_EQ(index, expected);
    }
  }
}
