#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/operation/add.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/sub.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace arithmetic_executor_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe>
struct ParameterSet {
  using rlwe_params = Rlwe;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace arithmetic_executor_test

template <typename Ctx>
class ArithmeticFixture : public ::testing::Test {
 protected:
  using Rlwe = typename Ctx::context::rlwe_params;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  struct TestCase {
    Poly<rTorus, N> lhs;
    Poly<rTorus, N> rhs;
  };

  Runtime<Cryptor<Rlwe>, Tracking> rlwe_runtime_;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  void SetUp() override {
    SecretHolder<Rlwe> kr(eng_);
    rlwe_runtime_ = Runtime<Cryptor<Rlwe>, Tracking>(kr.secret_ptr(), eng_);
  }

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    // Fixed corner cases
    {
      TestCase tc;
      for (size_t i = 0; i < N; ++i) {
        tc.lhs = Poly<rTorus, N>();
        tc.rhs = Poly<rTorus, N>();
      }
      cases.push_back(std::move(tc));  // all zeros
    }
    {
      TestCase tc;
      tc.lhs[0] = rTorus(1u);
      tc.rhs[0] = rTorus(rTorus::raw_max());
      cases.push_back(std::move(tc));
    }
    // Random cases
    std::uniform_int_distribution<typename rTorus::raw_value_type> dist;
    for (int k = 0; k < 2; ++k) {
      TestCase tc;
      randomize(tc.lhs, eng_);
      randomize(tc.rhs, eng_);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(ArithmeticFixture, arithmetic_executor_test::TestContexts);

TYPED_TEST(ArithmeticFixture, AdditionCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  for (const auto& tc : this->cases()) {
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

  for (const auto& tc : this->cases()) {
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