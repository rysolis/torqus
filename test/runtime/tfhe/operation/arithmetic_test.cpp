#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/operation/add.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/sub.hpp"
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

using Context1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Context2 =
    ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

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
        Evaluator<Add<Rlwe>, Tracking>::exec(lhs_ct, rhs_ct);
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = lhs + rhs;

    Poly<rTorus, N> err = res - ref;
    double norm = infinity_norm(err);

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
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
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
        Evaluator<Sub<Rlwe>, Tracking>::exec(lhs_ct, rhs_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = lhs - rhs;

    // compute actual result
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    Poly<rTorus, N> err = ref - res;
    double norm = infinity_norm(err);

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
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}