#include "tfhe/circuit/binary_expansion.hpp"
#include <gtest/gtest.h>

#include <iomanip>

#include "primitive/torus.hpp"

#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

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
  using rTorus = Base::rTorus;

  struct TestCase {
    Vector<Torus, 2> operand;
    Vector<rTorus, 4> ref;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.operand = {Torus(0u), Torus(0u)},
             .ref = {rTorus(1u, 4u), rTorus(0u), rTorus(0u), rTorus(0u)}},
            {.operand = {Torus(1u, 4u), Torus(0u)},
             .ref = {rTorus(0u), rTorus(1u, 4u), rTorus(0u), rTorus(0u)}},
            {.operand = {Torus(0u), Torus(1u, 4u)},
             .ref = {rTorus(0u), rTorus(0u), rTorus(1u, 4u), rTorus(0u)}},
            {.operand = {Torus(1u, 4u), Torus(1u, 4u)},
             .ref = {rTorus(0u), rTorus(0u), rTorus(0u), rTorus(1u, 4u)}}};
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
    // compute reference result
    Vector<rTorus, 4> ref = tc.ref;

    // compute actual result
    Vector<rTorus, 4> res;
    res[0] = this->rlwe_runtime_.decrypt(res_ct[0]);
    res[1] = this->rlwe_runtime_.decrypt(res_ct[1]);
    res[2] = this->rlwe_runtime_.decrypt(res_ct[2]);
    res[3] = this->rlwe_runtime_.decrypt(res_ct[3]);

    Vector<rTorus, 4> err = res - ref;
    double norm = infinity_norm(err);

    // Output lives at {0, 1/4} mod 1; a wrong decode only happens past
    // half that gap, so that's the margin "did it decode right" is
    // judged against here.
    const double decode_margin = double(rTorus(1u, 4u)) / 2;

    std::cout << "\n========================================\n";
    std::cout << "         BinaryExpansion Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "operand" << ": " << operand << "\n";
    std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res << "\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';

    EXPECT_LE(norm, decode_margin);
  }
}
