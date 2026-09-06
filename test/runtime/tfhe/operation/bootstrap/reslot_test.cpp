#include <gtest/gtest.h>

#include <iomanip>

#include "tfhe/bit.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/operation/bootstrap/reslot.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace reslot_test {
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
}  // namespace reslot_test

template <typename Context>
class ReslotFixture : public ::testing::Test {
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
class ReslotCorrectnessTest : public ReslotFixture<typename Config::context> {
 protected:
  struct TestCase {
    bool value;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.value = true}, {.value = false}};
  }
};

TYPED_TEST_SUITE(ReslotCorrectnessTest, reslot_test::TestContexts);

// Dial<2, Torus> (0 or 1/2) moved into Dial<4, Torus> (0 or 1/4) -- the
// step And/Or/AndNot/Xor expect.
TYPED_TEST(ReslotCorrectnessTest, MovesValueFromOneResolutionToAnother) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  Boundary<2, Lwe, Rlwe, Decomp, Tracking> in_boundary(this->lwe_runtime_,
                                                       this->rlwe_runtime_);
  Boundary<4, Lwe, Rlwe, Decomp, Tracking> out_boundary(this->lwe_runtime_,
                                                        this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Bit<Lwe, Rlwe> ct = in_boundary.lift(tc.value);

    // ==================================
    // Act
    // ==================================
    Bit<Lwe, Rlwe> res_ct(
        tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, 2, 4>::exec_impl(ct.ready(),
                                                                    this->BK_));

    // ==================================
    // Assert
    // ==================================
    bool res = out_boundary.drop(res_ct);

    std::cout << "\n========================================\n";
    std::cout << "           Reslot Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "value" << ": " << tc.value << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res << "\n";

    EXPECT_EQ(res, tc.value);
  }
}

// OutResolution has no power-of-two constraint (only InResolution does, so
// scaling up to exactly 1/2 by repeated self-addition lands on an integer
// number of doublings) -- Dial<4, Torus> (0 or 1/4) moved into Dial<100,
// Torus> (0 or 1/100) exercises that.
TYPED_TEST(ReslotCorrectnessTest, OutResolutionNeedNotBeAPowerOfTwo) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  Boundary<4, Lwe, Rlwe, Decomp, Tracking> in_boundary(this->lwe_runtime_,
                                                       this->rlwe_runtime_);
  Boundary<100, Lwe, Rlwe, Decomp, Tracking> out_boundary(this->lwe_runtime_,
                                                          this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    Bit<Lwe, Rlwe> ct = in_boundary.lift(tc.value);

    Bit<Lwe, Rlwe> res_ct(
        tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, 4, 100>::exec_impl(
            ct.ready(), this->BK_));

    uint32_t res = out_boundary.drop(res_ct);

    std::cout << "\n========================================\n";
    std::cout << "           Reslot Test (4 -> 100)\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "value" << ": " << tc.value << "\n";
    std::cout << std::setw(14) << "actual index" << ": " << res << "\n";

    // Dial<100,...>'s indices 0/1 are still 0 and 1/100 -- same true/false
    // reading as Dial<4,...>'s 0/1, just on a finer grid.
    EXPECT_EQ(res, tc.value ? 1u : 0u);
  }
}
