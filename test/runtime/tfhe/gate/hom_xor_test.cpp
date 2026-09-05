#include <gtest/gtest.h>

#include "tfhe/bit.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/lift.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace hom_xor_test {
template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe, typename Decomp>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
  using dcp_params = Decomp;
};

// Real noise enabled (see gate_bootstrap_test.cpp's Context).
using Context = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>,
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>,
    dcp_params<16, 7>>;

using TestContexts = ::testing::Types<TestConfig<Context>>;
}  // namespace hom_xor_test

template <typename Context>
class HomXorFixture : public ::testing::Test {
 protected:
  using Lwe = Context::lwe_params;
  using Rlwe = Context::rlwe_params;
  using Decomp = Context::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = typename Lwe::torus_type;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Lwe, Tracking> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Tracking> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> BK_;

  void SetUp() override {
    lwe_runtime_ = Runtime<Lwe, Tracking>(eng_);
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>, Tracking>(eng_);

    // Prepare Bootstrapkey
    BK_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime_.holder().get());
  }
};

template <typename Config>
class HomXorCorrectnessTest : public HomXorFixture<typename Config::context> {
 protected:
  struct TestCase {
    bool lhs;
    bool rhs;
    bool ref;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.lhs = true, .rhs = true, .ref = false},
            {.lhs = false, .rhs = true, .ref = true},
            {.lhs = true, .rhs = false, .ref = true},
            {.lhs = false, .rhs = false, .ref = false}};
  }
};

TYPED_TEST_SUITE(HomXorCorrectnessTest, hom_xor_test::TestContexts);

TYPED_TEST(HomXorCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  Lift<4, Lwe, Rlwe, Tracking> lift(this->lwe_runtime_);
  Drop<4, Lwe, Rlwe, Decomp, Tracking> drop(this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Bit<Lwe, Rlwe> lhs_ct = lift.encrypt(tc.lhs);
    Bit<Lwe, Rlwe> rhs_ct = lift.encrypt(tc.rhs);

    // ==================================
    // Act
    // ==================================
    Bit<Lwe, Rlwe> res_ct(tfhe::gate::HomXor<Lwe, Rlwe, Decomp>::exec_impl(
        lhs_ct.ready(), rhs_ct.ready(), this->BK_));

    // ==================================
    // Assert
    // ==================================
    bool res = drop.decrypt(res_ct);

    std::cout << "\n========================================\n";
    std::cout << "           HomXor Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "lhs" << ": " << tc.lhs << "\n";
    std::cout << std::setw(14) << "rhs" << ": " << tc.rhs << "\n";
    std::cout << std::setw(14) << "expected" << ": " << tc.ref << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res << "\n";

    EXPECT_EQ(res, tc.ref);
  }
}
