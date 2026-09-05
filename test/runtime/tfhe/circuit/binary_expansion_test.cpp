#include "tfhe/circuit/binary_expansion.hpp"
#include <gtest/gtest.h>

#include <iomanip>

#include "primitive/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/bit.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/lift.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/scope.hpp"
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

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe, Tracking> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Tracking> rlwe_runtime_;

  Circuit<Lwe, Rlwe, Decomp> circuit_;
  Relay<Lwe, Rlwe, Kst> relay_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>, Tracking>(eng_);
    lwe_runtime_ = Runtime<Lwe, Tracking>(eng_);

    circuit_ = Circuit<Lwe, Rlwe, Decomp>(
        rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
            lwe_runtime_.holder().get()));
    relay_ = Relay<Lwe, Rlwe, Kst>(
        lwe_runtime_
            .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                rlwe_runtime_.holder().get()));
  }
};

template <typename Config>
class BinaryExpansionCorrectnessTest
    : public BinaryExpansionFixture<typename Config::context> {
 protected:
  // BinaryExpansion<4, ...> turns a 2-bit operand into a one-hot 4-output
  // vector -- exactly one output (at index `hot`) decodes to true, the
  // rest to false.
  struct TestCase {
    bool a;
    bool b;
    uint32_t hot;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.a = false, .b = false, .hot = 0},
            {.a = true, .b = false, .hot = 1},
            {.a = false, .b = true, .hot = 2},
            {.a = true, .b = true, .hot = 3}};
  }
};

TYPED_TEST_SUITE(BinaryExpansionCorrectnessTest,
                 binary_expansion_test::TestContexts);

TYPED_TEST(BinaryExpansionCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;
  using Kst = typename TypeParam::context::kst_params;

  Lift<4, Lwe, Rlwe, Tracking> lift(this->lwe_runtime_);
  Drop<4, Lwe, Rlwe, Decomp, Tracking> drop(this->rlwe_runtime_);

  tfhe::circuit::BinaryExpansion<4, Lwe, Rlwe, Decomp, Kst> expansion(
      this->circuit_, this->relay_);

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Vector<TLWE<typename Lwe::torus_type, Lwe::n>, 2> operand_ct;
    operand_ct[0] = lift.encrypt(tc.a).ready();
    operand_ct[1] = lift.encrypt(tc.b).ready();

    // ==================================
    // Act
    // ==================================
    Vector<TLWE<typename Rlwe::torus_type, Rlwe::N>, 4> res_ct =
        expansion.exec_impl(operand_ct);

    // ==================================
    // Assert
    // ==================================
    std::cout << "\n========================================\n";
    std::cout << "         BinaryExpansion Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "operand" << ": (" << tc.a << ", " << tc.b
              << ")\n";
    std::cout << std::setw(14) << "hot index" << ": " << tc.hot << "\n";

    for (uint32_t i = 0; i < 4; ++i) {
      bool res = drop.decrypt(Bit<Lwe, Rlwe>(res_ct[i]));
      bool expected = (i == tc.hot);
      EXPECT_EQ(res, expected);
    }
  }
}
