#include "tfhe/operation/bootstrap/blindrotate.hpp"
#include <gtest/gtest.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <random>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/random_generator.hpp"
#include "tfhe/utility/testvector.hpp"

namespace blindrotate_test {

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

// noise_params on Rlwe add real bootstrap-key noise -- Context2 uses the
// same 2^-25 as the paper's own 128-bit N=1024 parameter (see
// gate_bootstrap_test.cpp).
using Context1 = ParameterSet<
    lwe_params<tlwe_core_params<void, 1>>,
    rlwe_params<trlwe_core_params<ModTorus<16>, 4>, noise_params<11>>,
    dcp_params<4, 3>>;

using Context2 = ParameterSet<
    lwe_params<tlwe_core_params<void, 630>>,
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>,
    dcp_params<256, 3>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace blindrotate_test

template <typename Context>
class BlindRotateFixture : public ::testing::Test {
 protected:
  using Lwe = typename Context::lwe_params;
  using Rlwe = typename Context::rlwe_params;
  using Decomp = typename Context::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Decomp::l;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{10};

  Runtime<Rlwe, Tracking> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> BK_;
  Vector<ModInt<M>, n + 1> phase_ct_;

  void SetUp() override {
    Runtime<Lwe> lwe_runtime = Runtime<Lwe>(eng_);
    rlwe_runtime_ = Runtime<Rlwe, Tracking>(eng_);

    // Prepare Bootstrapkey
    BK_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
        lwe_runtime.holder().get());

    // Prepare Vector<ModInt<M>, n + 1> phase_ct;
    randomize(phase_ct_, this->eng_);

    ModInt<M> b{};
    for (size_t i = 0; i < n; ++i) {
      b += static_cast<UInt>(lwe_runtime.holder().get()[i]) *
           static_cast<ModInt<M>>(phase_ct_[i]);
    }

    phase_ct_[n] = b;  // Overwrite!
  }
};

template <typename Config>
class BlindRotateCorrectnessTest
    : public BlindRotateFixture<typename Config::context> {
 protected:
  using Base = BlindRotateFixture<typename Config::context>;

  using rTorus = typename Base::rTorus;
  static constexpr uint32_t M = Base::M;

  struct TestCase {
    rTorus mu = rTorus(1u, 2u);  // encode 1/2 in rTorus
    ModInt<M> phase;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.phase = ModInt<M>(0)},
            {.phase = ModInt<M>(1)},
            {.phase = ModInt<M>(M - 1)},
            {.phase = ModInt<M>(M / 2)}};
  }
};

TYPED_TEST_SUITE(BlindRotateCorrectnessTest, blindrotate_test::TestContexts);

TYPED_TEST(BlindRotateCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    rTorus mu = tc.mu;
    ModInt<M> phase = tc.phase;

    // Prepare Vector<ModInt<M>, n+1> (contains phase)
    Vector<ModInt<M>, n + 1> phase_ct = this->phase_ct_;
    phase_ct[n] = static_cast<ModInt<M>>(phase_ct[n]) + phase;

    // Prepare TestVector
    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu.value() >> 1u));

    // ==================================
    // Act
    // ==================================
    TRLWE<rTorus, N> res_ct = tfhe::operation::Evaluator<
        tfhe::bootstrap::BlindRotate<Lwe, Rlwe, Decomp>,
        Tracking>::exec(tv, phase_ct, this->BK_);
    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = rotate(tv.b(), (-phase).value());

    // compute actual result
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    Poly<rTorus, N> err = ref - res;
    double norm = infinity_norm(err);

    // 99% two-sided normal threshold on VarianceNoisePolicy's predicted
    // stddev, alongside the worst-case NoisePolicy check below --
    // res_ct's error is an N-coefficient polynomial, so
    // confidence_threshold(res_ct, N); see gate_bootstrap_test.cpp and
    // tracker_if.hpp for the full rationale.
    double variance_threshold =
        get_variance_tracker_if()->confidence_threshold(res_ct, N);

    std::cout << "\n========================================\n";
    std::cout << "           BlindRotate Test\n";
    std::cout << "========================================\n";

    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "tv" << ": " << tv.b() << '\n';
      std::cout << std::setw(14) << "expected" << ": " << ref << '\n';
      std::cout << std::setw(14) << "actual" << ": " << res << '\n';
    }

    std::cout << std::left;
    std::cout << std::setw(14) << "mu" << ": " << mu << '\n';
    std::cout << std::setw(14) << "phase " << ": " << phase << '\n';
    std::cout << std::setw(14) << "-phase" << ": " << -phase << '\n';
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
    std::cout << std::setw(14) << "errror_bound " << ": "
              << get_noise_tracker_if()->get(res_ct) << '\n';
    std::cout << std::setw(14) << "99% threshold" << ": " << variance_threshold
              << '\n';

    std::cout << "========================================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
    EXPECT_LE(norm, variance_threshold);
  }
}