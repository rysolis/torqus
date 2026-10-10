#include "tfhe/circuit/binary_expansion.hpp"
#include <gtest/gtest.h>

#include <array>
#include <iomanip>
#include <vector>

#include "primitive/torus.hpp"

#include "tfhe/cipher.hpp"
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

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;
  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe, Tracking> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Tracking> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> bk_;
  KeySwitchKey<Torus, n, t, N> ksk_;
  tfhe::circuit::BinaryExpansion<4, Lwe, Rlwe, Decomp, Kst> expansion_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>, Tracking>(eng_);
    lwe_runtime_ = Runtime<Lwe, Tracking>(eng_);

    bk_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime_.secret());
    ksk_ = lwe_runtime_
               .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                   rlwe_runtime_.secret());
    expansion_ =
        tfhe::circuit::BinaryExpansion<4, Lwe, Rlwe, Decomp, Kst>(bk_, ksk_);
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

  Boundary<Lwe, Rlwe, Decomp, Tracking> boundary(this->lwe_runtime_,
                                                 this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    std::vector<TLWE<typename Lwe::torus_type, Lwe::n>> operand_ct;
    operand_ct.push_back(boundary.template lift<4>(tc.a));
    operand_ct.push_back(boundary.template lift<4>(tc.b));

    // ==================================
    // Act
    // ==================================
    std::array<Cipher<Lwe, Rlwe>, 4> res_ct = this->expansion_.exec(operand_ct);

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
      bool res = boundary.template drop<4>(res_ct[i]);
      bool expected = (i == tc.hot);
      EXPECT_EQ(res, expected);
    }
  }
}

// materialize() does the KeySwitch itself -- each exec() output converts
// to Lwe-shaped in place.
TYPED_TEST(BinaryExpansionCorrectnessTest, MaterializeMakesAllSlotsReady) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  Boundary<Lwe, Rlwe, Decomp, Tracking> boundary(this->lwe_runtime_,
                                                 this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    std::vector<TLWE<typename Lwe::torus_type, Lwe::n>> operand_ct;
    operand_ct.push_back(boundary.template lift<4>(tc.a));
    operand_ct.push_back(boundary.template lift<4>(tc.b));

    std::array<Cipher<Lwe, Rlwe>, 4> res_ct = this->expansion_.exec(operand_ct);

    std::cout << "\n========================================\n";
    std::cout << "     BinaryExpansion materialize Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "operand" << ": (" << tc.a << ", " << tc.b
              << ")\n";
    std::cout << std::setw(14) << "hot index" << ": " << tc.hot << "\n";

    for (uint32_t i = 0; i < 4; ++i) {
      bool res =
          boundary.template drop<4>(this->expansion_.materialize(res_ct[i]));
      bool expected = (i == tc.hot);
      EXPECT_EQ(res, expected);
    }
  }
}

// Same as above, but materializing one slot at a time via exec_slot_impl --
// a caller farming slots across its own thread pool instead of computing
// all H at once.
TYPED_TEST(BinaryExpansionCorrectnessTest, MaterializeMakesOneSlotReady) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  Boundary<Lwe, Rlwe, Decomp, Tracking> boundary(this->lwe_runtime_,
                                                 this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    std::vector<TLWE<typename Lwe::torus_type, Lwe::n>> operand_ct;
    operand_ct.push_back(boundary.template lift<4>(tc.a));
    operand_ct.push_back(boundary.template lift<4>(tc.b));

    std::cout << "\n========================================\n";
    std::cout << "  BinaryExpansion materialize (1 slot) Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "operand" << ": (" << tc.a << ", " << tc.b
              << ")\n";
    std::cout << std::setw(14) << "hot index" << ": " << tc.hot << "\n";

    for (uint32_t h = 0; h < 4; ++h) {
      Cipher<Lwe, Rlwe> res_ct = this->expansion_.exec_slot_impl(h, operand_ct);
      bool res =
          boundary.template drop<4>(this->expansion_.materialize(res_ct));
      bool expected = (h == tc.hot);
      EXPECT_EQ(res, expected);
    }
  }
}

// this->expansion_ has FuseKeySwitch=false (the default): each
// exec_slot_impl step calls Backend's plain exec_impl and relay_ does the
// KeySwitch. fused_expansion below has FuseKeySwitch=true instead, same
// Backend (GateBootstrap) -- same bk_/ksk_, same inputs, confirming both
// branches agree bit-for-bit.
TYPED_TEST(BinaryExpansionCorrectnessTest, FusedKeySwitchMatchesDefault) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;
  using Kst = typename TypeParam::context::kst_params;
  using Torus = typename Lwe::torus_type;

  Boundary<Lwe, Rlwe, Decomp, Tracking> boundary(this->lwe_runtime_,
                                                 this->rlwe_runtime_);
  tfhe::circuit::BinaryExpansion<4, Lwe, Rlwe, Decomp, Kst,
                                 tfhe::bootstrap::GateBootstrap, true>
      fused_expansion(this->bk_, this->ksk_);

  for (const auto& tc : TestFixture::cases()) {
    std::vector<TLWE<Torus, Lwe::n>> operand_ct;
    operand_ct.push_back(boundary.template lift<4>(tc.a));
    operand_ct.push_back(boundary.template lift<4>(tc.b));

    std::array<Cipher<Lwe, Rlwe>, 4> expected =
        this->expansion_.exec(operand_ct);
    std::array<Cipher<Lwe, Rlwe>, 4> actual = fused_expansion.exec(operand_ct);

    for (uint32_t h = 0; h < 4; ++h) {
      const TLWE<Torus, Lwe::n>& expected_ready =
          this->expansion_.materialize(expected[h]);
      const TLWE<Torus, Lwe::n>& actual_ready =
          fused_expansion.materialize(actual[h]);

      EXPECT_EQ(actual_ready.b(), expected_ready.b());
      for (uint32_t j = 0; j < Lwe::n; ++j) {
        EXPECT_EQ(Torus(actual_ready.a()[j]), Torus(expected_ready.a()[j]));
      }
    }
  }
}
