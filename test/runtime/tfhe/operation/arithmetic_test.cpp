#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/operation/add.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/operation/sub.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
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

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  TrackedCryptor<Cryptor<Rlwe>> cryptor_;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  void SetUp() override {
    SecretHolder<Rlwe> kr(eng_);
    cryptor_ = TrackedCryptor<Cryptor<Rlwe>>(kr.secret_ptr(), eng_);
  }
};

TYPED_TEST_SUITE(ArithmeticFixture, arithmetic_executor_test::TestContexts);

TYPED_TEST(ArithmeticFixture, AdditionCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  Poly<Torus, N> lhs, rhs;
  lhs[0] = Torus(1);
  rhs[0] = Torus(100);

  // ==================================
  Poly<Torus, N> expected = lhs + rhs;
  // ----------------------------------
  TRLWE<Torus, N> encrypted_lhs = this->cryptor_.encrypt(lhs);
  TRLWE<Torus, N> encrypted_rhs = this->cryptor_.encrypt(rhs);

  TRLWE<Torus, N> encrypted =
      Evaluator<Add<Rlwe>, Tracking>::exec(encrypted_lhs, encrypted_rhs);

  Poly<Torus, N> decrypted = this->cryptor_.decrypt(encrypted);
  // ==================================

  Poly<Torus, N> err = decrypted - expected;
  double norm = infinity_norm(err);

  std::cout << "\n=== Add Executor Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "decrypted" << ": " << decrypted << "\n";
    std::cout << std::setw(14) << "expected" << ": " << expected << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
  std::cout << std::setw(14) << "error_bound" << ": "
            << get_noise_tracker_if()->get(encrypted) << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(encrypted));
}

TYPED_TEST(ArithmeticFixture, SubtractionCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  Poly<Torus, N> lhs, rhs;
  lhs[0] = Torus(1);
  rhs[0] = Torus(100);

  // ==================================
  Poly<Torus, N> expected = lhs - rhs;
  // ----------------------------------
  TRLWE<Torus, N> encrypted_lhs = this->cryptor_.encrypt(lhs);
  TRLWE<Torus, N> encrypted_rhs = this->cryptor_.encrypt(rhs);

  TRLWE<Torus, N> encrypted =
      Evaluator<Sub<Rlwe>, Tracking>::exec(encrypted_lhs, encrypted_rhs);

  Poly<Torus, N> decrypted = this->cryptor_.decrypt(encrypted);
  // ==================================

  Poly<Torus, N> err = decrypted - expected;
  double norm = infinity_norm(err);

  std::cout << "\n=== Sub Executor Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "decrypted" << ": " << decrypted << "\n";
    std::cout << std::setw(14) << "expected" << ": " << expected << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
  std::cout << std::setw(14) << "error_bound" << ": "
            << get_noise_tracker_if()->get(encrypted) << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(encrypted));
}