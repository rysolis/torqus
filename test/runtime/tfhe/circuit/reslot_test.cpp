#include "tfhe/circuit/reslot.hpp"
#include <gtest/gtest.h>

#include <iomanip>

#include "tfhe/cipher.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace circuit_reslot_test {
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

// Real noise enabled (see gate_bootstrap_test.cpp's Context).
using Context = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>,
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>,
    dcp_params<16, 7>, kst_params<2, 11>>;

using TestContexts = ::testing::Types<TestConfig<Context>>;
}  // namespace circuit_reslot_test

template <typename Context>
class CircuitReslotFixture : public ::testing::Test {
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
  tfhe::circuit::Reslot<2, 4, Lwe, Rlwe, Decomp, Kst> reslot_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<ParamsPack<Rlwe, Decomp>, Tracking>(eng_);
    lwe_runtime_ = Runtime<Lwe, Tracking>(eng_);

    bk_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime_.secret());
    ksk_ = lwe_runtime_
               .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                   rlwe_runtime_.secret());
    reslot_ = tfhe::circuit::Reslot<2, 4, Lwe, Rlwe, Decomp, Kst>(bk_, ksk_);
  }
};

template <typename Config>
class CircuitReslotCorrectnessTest
    : public CircuitReslotFixture<typename Config::context> {
 protected:
  struct TestCase {
    bool value;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.value = true}, {.value = false}};
  }
};

TYPED_TEST_SUITE(CircuitReslotCorrectnessTest,
                 circuit_reslot_test::TestContexts);

// A ciphertext lifted at Dial<2, Torus> (0 or 1/2) -- e.g. a ballot bit
// encoded outside this library's own gate suite -- comes back moved to
// Dial<4, Torus> (0 or 1/4), the step HomAnd/HomOr/HomAndNot/HomXor expect.
// exec_ready() does the bootstrap plus a Relay::materialize() in one call
// -- no separate Relay needed by the caller (see Reslot's own doc comment;
// exec() alone staying Rlwe-shaped/pending is covered by CipherTest's own
// Reslot* tests, which use the same underlying tfhe::bootstrap::Reslot).
TYPED_TEST(CircuitReslotCorrectnessTest, MovesAndMaterializesInOneCall) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  Boundary<2, Lwe, Rlwe, Decomp, Tracking> in_boundary(this->lwe_runtime_,
                                                       this->rlwe_runtime_);
  Boundary<4, Lwe, Rlwe, Decomp, Tracking> out_boundary(this->lwe_runtime_,
                                                        this->rlwe_runtime_);

  for (const auto& tc : TestFixture::cases()) {
    Cipher<Lwe, Rlwe> ct = in_boundary.lift(tc.value);

    TLWE<typename Lwe::torus_type, Lwe::n> ready = this->reslot_.exec_ready(ct);

    bool res = out_boundary.drop(ready);

    std::cout << "\n========================================\n";
    std::cout << "         Circuit::Reslot Test\n";
    std::cout << "========================================\n";

    std::cout << std::left;
    std::cout << std::setw(14) << "value" << ": " << tc.value << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res << "\n";

    EXPECT_EQ(res, tc.value);
  }
}
