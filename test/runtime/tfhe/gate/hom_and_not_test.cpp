#include <gtest/gtest.h>

#include "algebra/utility/utility.hpp"

#include "tfhe/dial.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/operation/leveled/sub.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace hom_and_not_test {
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
}  // namespace hom_and_not_test

template <typename Context>
class HomAndNotFixture : public ::testing::Test {
 protected:
  using Lwe = Context::lwe_params;
  using Rlwe = Context::rlwe_params;
  using Decomp = Context::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = typename Lwe::torus_type;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

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
class HomAndNotCorrectnessTest
    : public HomAndNotFixture<typename Config::context> {
 protected:
  using Base = HomAndNotFixture<typename Config::context>;

  using Torus = Base::Torus;
  using rTorus = Base::rTorus;

  using Bit = Dial<4, Torus>;
  using rBit = Dial<4, rTorus>;

  struct TestCase {
    Torus lhs;
    Torus rhs;
    rTorus ref;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {
        {.lhs = Bit::at(true), .rhs = Bit::at(true), .ref = rBit::at(false)},
        {.lhs = Bit::at(false), .rhs = Bit::at(true), .ref = rBit::at(false)},
        {.lhs = Bit::at(true), .rhs = Bit::at(false), .ref = rBit::at(true)},
        {.lhs = Bit::at(false), .rhs = Bit::at(false), .ref = rBit::at(false)}};
  }
};

TYPED_TEST_SUITE(HomAndNotCorrectnessTest, hom_and_not_test::TestContexts);

TYPED_TEST(HomAndNotCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  using rBit = Dial<4, rTorus>;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    // Prepare TLWE
    Torus lhs = tc.lhs;
    Torus rhs = tc.rhs;
    TLWE<Torus, n> lhs_ct = this->lwe_runtime_.encrypt(lhs);
    TLWE<Torus, n> rhs_ct = this->lwe_runtime_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TLWE<rTorus, N> res_ct =
        tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp>::exec_impl(lhs_ct, rhs_ct,
                                                            this->BK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    rTorus ref = tc.ref;

    // comput actual result
    rTorus res = this->rlwe_runtime_.decrypt(res_ct);

    rTorus err = res - ref;
    double norm = infinity_norm(err);

    // Output lives at rBit's {0, 1/4} encoding; a wrong decode only
    // happens past half that gap, so that's the margin "did it decode
    // right" is judged against here.
    const double decode_margin = double(rBit::margin());

    std::cout << "\n========================================\n";
    std::cout << "           HomAndNot Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
    std::cout << std::setw(14) << "rhs" << ": " << rhs << "\n";
    std::cout << std::setw(14) << "expected" << ": " << ref << " ("
              << double(ref) << ")\n";
    std::cout << std::setw(14) << "actual" << ": " << res << " (" << double(res)
              << ")\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';

    EXPECT_LE(norm, decode_margin);
  }
}
