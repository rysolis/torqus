#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/operation/leveled/sub.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace arithmetic_executor_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe>
struct ParameterSet {
  using rlwe_params = Rlwe;
};

// noise_params here are real alpha, not just for show: Add/Sub's own
// NoisePolicy/VarianceNoisePolicy have no key-noise amplification term (no
// coefficient multiplying a tracked bound/variance by n*N*B-scale factors,
// unlike ExternalProduct/CMux/BlindRotate/KeySwitch), so any alpha is safe
// here regardless of N -- Context2 uses the same 2^-25 as the paper's own
// 128-bit N=1024 parameter (see gate_bootstrap_test.cpp) for consistency.
using Context1 = ParameterSet<
    rlwe_params<trlwe_core_params<ModTorus<16>, 4>, noise_params<11>>>;
using Context2 = ParameterSet<
    rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace arithmetic_executor_test

template <typename Context>
class ArithmeticFixture : public ::testing::Test {
 protected:
  using Rlwe = typename Context::context::rlwe_params;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Rlwe, Tracking> rlwe_runtime_;

  void SetUp() override { rlwe_runtime_ = Runtime<Rlwe, Tracking>(eng_); }

  struct TestCase {
    Poly<rTorus, N> lhs;
    Poly<rTorus, N> rhs;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.lhs = Poly<rTorus, N>(), .rhs = Poly<rTorus, N>()},
            {.lhs = Poly<rTorus, N>([] { return rTorus(1); }),
             .rhs = Poly<rTorus, N>([] { return rTorus::raw_max(); })},
            {.lhs = randomize<Poly<rTorus, N>>(this->eng_),
             .rhs = randomize<Poly<rTorus, N>>(this->eng_)}};
  }
};

TYPED_TEST_SUITE(ArithmeticFixture, arithmetic_executor_test::TestContexts);

TYPED_TEST(ArithmeticFixture, AdditionCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Poly<rTorus, N> lhs, rhs;
    lhs = tc.lhs;
    rhs = tc.rhs;

    TRLWE<rTorus, N> lhs_ct = this->rlwe_runtime_.encrypt(lhs);
    TRLWE<rTorus, N> rhs_ct = this->rlwe_runtime_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TRLWE<rTorus, N> res_ct =
        tfhe::operation::Evaluator<tfhe::leveled::Add<Rlwe>, Tracking>::exec(
            lhs_ct, rhs_ct);
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = lhs + rhs;

    Poly<rTorus, N> err = res - ref;
    double norm = infinity_norm(err);

    // Union-bound-corrected 99% threshold (see tracker_if.hpp's
    // confidence_threshold) on VarianceNoisePolicy's predicted stddev,
    // alongside the worst-case NoisePolicy check below -- necessary because
    // norm is a max over N independent coefficients, not a single sample;
    // see gate_bootstrap_test.cpp for the full rationale.
    double variance_threshold =
        get_variance_tracker_if()->confidence_threshold(res_ct, N);

    std::cout << "\n=== Add Executor Test ===\n";
    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
      std::cout << std::setw(14) << "rhs" << ": " << rhs << "\n";
      std::cout << std::setw(14) << "actual" << ": " << res << "\n";
      std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
    }
    std::cout << std::left;
    std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
    std::cout << std::setw(14) << "error_bound" << ": "
              << get_noise_tracker_if()->get(res_ct) << "\n";
    std::cout << std::setw(14) << "99% threshold" << ": " << variance_threshold
              << "\n";
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
    EXPECT_LE(norm, variance_threshold);
  }
}

TYPED_TEST(ArithmeticFixture, SubtractionCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    Poly<rTorus, N> lhs, rhs;
    lhs = tc.lhs;
    rhs = tc.rhs;

    TRLWE<rTorus, N> lhs_ct = this->rlwe_runtime_.encrypt(lhs);
    TRLWE<rTorus, N> rhs_ct = this->rlwe_runtime_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TRLWE<rTorus, N> res_ct =
        tfhe::operation::Evaluator<tfhe::leveled::Sub<Rlwe>, Tracking>::exec(
            lhs_ct, rhs_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = lhs - rhs;

    // compute actual result
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    Poly<rTorus, N> err = ref - res;
    double norm = infinity_norm(err);

    double variance_threshold =
        get_variance_tracker_if()->confidence_threshold(res_ct, N);

    std::cout << "\n=== Sub Executor Test ===\n";
    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
      std::cout << std::setw(14) << "rhs" << ": " << rhs << "\n";
      std::cout << std::setw(14) << "actual" << ": " << res << "\n";
      std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
    }
    std::cout << std::left;
    std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
    std::cout << std::setw(14) << "error_bound" << ": "
              << get_noise_tracker_if()->get(res_ct) << "\n";
    std::cout << std::setw(14) << "99% threshold" << ": " << variance_threshold
              << "\n";
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
    EXPECT_LE(norm, variance_threshold);
  }
}