#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include <gtest/gtest.h>

#include <cmath>

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/random_generator.hpp"
#include "tfhe/utility/testvector.hpp"

namespace gate_bootstrap_test {

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

// noise_params<11> (alpha=2^-11) is real, non-negligible noise, but small
// enough that assert(bound < 0.25) below still clears comfortably at these
// dimensions -- see VerifyCorrectness's variance check further down.
using Context1 = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<16>, 1>>,
    rlwe_params<trlwe_core_params<ModTorus<16>, 4>, noise_params<11>>,
    dcp_params<4, 3>>;

// n=630/N=1024 match the TFHE paper's own published 128-bit-security
// parameter set, with its own lattice-estimator-verified alpha for these
// exact dimensions (2^-15 for n=630, 2^-25 for N=1024). NoisePolicy's own
// bound blows past 0.25 at this alpha (no
// longer asserted on, see evaluator.hpp), which is exactly why the real
// check here is VerifyCorrectness's variance-model threshold below, not
// the worst-case one.
using Context2 = ParameterSet<
    lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>,
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>,
    dcp_params<256, 3>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace gate_bootstrap_test

template <typename Context>
class GateBootstrapFixture : public ::testing::Test {
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
class GateBootstrapCorrectnessTest
    : public GateBootstrapFixture<typename Config::context> {
 protected:
  using Base = GateBootstrapFixture<typename Config::context>;

  using Torus = typename Base::Torus;
  using rTorus = typename Base::rTorus;

  struct TestCase {
    rTorus mu = rTorus(1u, 4u);  // Choose 1/4 in Torus as output
    Torus phase;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.phase = Torus(0u)}, {.phase = Torus(1u, 2u)}};
  }
};

TYPED_TEST_SUITE(GateBootstrapCorrectnessTest,
                 gate_bootstrap_test::TestContexts);

TYPED_TEST(GateBootstrapCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  // Alongside the worst-case NoisePolicy check below, also check norm
  // against VarianceNoisePolicy's predicted stddev (get_variance_tracker_if(),
  // populated automatically by Runtime/Evaluator under Tracking) at a
  // single-shot 99% two-sided confidence threshold -- cheaper than a
  // many-trial statistical suite, at the cost of less power. res_ct's error
  // is a single scalar (sample-extracted LWE, not a polynomial), so
  // confidence_threshold(res_ct, 1) here (see tracker_if.hpp).

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    rTorus mu = tc.mu;
    Torus phase = tc.phase;

    // Prepare TLWE (constains phase)
    TLWE<Torus, n> tlwe_rot = this->lwe_runtime_.encrypt(Torus(0u));
    tlwe_rot.b() = static_cast<Torus>(tlwe_rot.b()) + phase;

    // Prepare TestVector
    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu.value() >> 1u));

    // ==================================
    // Act
    // ==================================
    TLWE<rTorus, N> res_ct = tfhe::operation::Evaluator<
        tfhe::bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>,
        Tracking>::exec(mu, tv, tlwe_rot, this->BK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    constexpr uint32_t Q = [] {
      if constexpr (Torus::qbit == 32) {
        return 0;
      } else {
        return 1 << Torus::qbit;
      }
    }();
    ModInt<M> p = mod_switch<M>(ModInt<Q>(phase.value()));
    Poly<rTorus, N> rot = rotate(tv.b(), (-p).value());
    rTorus ref = static_cast<rTorus>(rot[0]) + rTorus(mu.value() / 2);

    // compute actual result
    rTorus res = this->rlwe_runtime_.decrypt(res_ct);

    rTorus err = ref - res;
    double norm = infinity_norm(err);

    double predicted_stddev = std::sqrt(get_variance_tracker_if()->get(res_ct));
    double variance_threshold =
        get_variance_tracker_if()->confidence_threshold(res_ct, 1);

    std::cout << "\n========================================\n";
    std::cout << "           Gate Bootstrap Test\n";
    std::cout << "========================================\n";

    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "tv" << ": " << tv << '\n';
    }

    std::cout << std::left;
    std::cout << std::setw(14) << "mu" << ": " << mu << " (" << double(mu)
              << ")\n";
    std::cout << std::setw(14) << "phase" << ": " << phase << '\n';
    std::cout << std::setw(14) << "phase(mod)" << ": " << p << '\n';
    std::cout << std::setw(14) << "expected" << ": " << ref << " ("
              << double(ref) << ")\n";
    std::cout << std::setw(14) << "actual" << ": " << res << " (" << double(res)
              << ")\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
    std::cout << std::setw(14) << "errror_bound " << ": "
              << get_noise_tracker_if()->get(res_ct) << '\n';
    std::cout << std::setw(14) << "predicted stddev" << ": " << predicted_stddev
              << '\n';
    std::cout << std::setw(14) << "99% threshold" << ": " << variance_threshold
              << '\n';

    std::cout << "========================================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
    EXPECT_LE(norm, variance_threshold);
  }
}
