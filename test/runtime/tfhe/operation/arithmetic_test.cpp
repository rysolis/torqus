#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/operation/add.hpp"
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

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace arithmetic_executor_test

template <typename Ctx>
class ArithmeticFixture : public ::testing::Test {
 protected:
  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  TrackedCryptor<Cryptor<params>> cryptor_;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  void SetUp() override {
    SecretHolder<params> kr(eng_);
    cryptor_ = TrackedCryptor<Cryptor<params>>(kr.secret_ptr(), eng_);
  }
};

TYPED_TEST_SUITE(ArithmeticFixture, arithmetic_executor_test::TestContexts);

TYPED_TEST(ArithmeticFixture, AdditionCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  Poly<Torus, N> lhs, rhs;
  lhs[0] = Torus(1);
  rhs[0] = Torus(100);

  // ==================================
  Poly<Torus, N> expected = lhs + rhs;
  // ----------------------------------
  TRLWE<Torus, N> encrypted_lhs = this->cryptor_.encrypt(lhs);
  TRLWE<Torus, N> encrypted_rhs = this->cryptor_.encrypt(rhs);

  TRLWE<Torus, N> encrypted =
      TrackedEvaluator<Add<params>>::exec(encrypted_lhs, encrypted_rhs);

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
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  Poly<Torus, N> lhs, rhs;
  lhs[0] = Torus(1);
  rhs[0] = Torus(100);

  // ==================================
  Poly<Torus, N> expected = lhs - rhs;
  // ----------------------------------
  TRLWE<Torus, N> encrypted_lhs = this->cryptor_.encrypt(lhs);
  TRLWE<Torus, N> encrypted_rhs = this->cryptor_.encrypt(rhs);

  TRLWE<Torus, N> encrypted =
      TrackedEvaluator<Sub<params>>::exec(encrypted_lhs, encrypted_rhs);

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