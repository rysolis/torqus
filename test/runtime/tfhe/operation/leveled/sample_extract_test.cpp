#include "tfhe/operation/leveled/sample_extract.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "algebra/utility/utility.hpp"

#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace sample_extract_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Glwe>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Glwe;
};

using Context1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 4>>,
                              rlwe_params<trlwe_core_params<ModTorus<32>, 4>>>;

using Context2 =
    ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 1024>>,
                 rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace sample_extract_test

template <typename Context>
class SampleExtractFixture : public ::testing::Test {
 protected:
  using Lwe = typename Context::lwe_params;
  using Rlwe = typename Context::rlwe_params;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Rlwe> rlwe_runtime_;
  Runtime<Lwe> lwe_runtime_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<Rlwe>(eng_);
    lwe_runtime_ = Runtime<Lwe>(rlwe_runtime_, eng_);
  }
};

template <typename Config>
class SampleExtractCorrectnessTest
    : public SampleExtractFixture<typename Config::context> {
 protected:
  using Base = SampleExtractFixture<typename Config::context>;

  using rTorus = typename Base::rTorus;
  static constexpr uint32_t N = Base::N;

  struct TestCase {
    size_t idx;
    Poly<rTorus, N> pt;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.idx = 0, .pt = randomize<Poly<rTorus, N>>(this->eng_)},
            {.idx = N / 2, .pt = randomize<Poly<rTorus, N>>(this->eng_)},
            {.idx = N - 1, .pt = randomize<Poly<rTorus, N>>(this->eng_)}};
  }
};

TYPED_TEST_SUITE(SampleExtractCorrectnessTest,
                 sample_extract_test::TestContexts);

TYPED_TEST(SampleExtractCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = typename Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  static_assert(n == N, "n and N must be equal for this test");

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    size_t idx = tc.idx;
    Poly<rTorus, N> pt = tc.pt;

    TRLWE<rTorus, N> pt_ct = this->rlwe_runtime_.encrypt(pt);

    // ==================================
    // Act
    // ==================================
    TLWE<Torus, n> res_ct = tfhe::operation::Evaluator<
        tfhe::leveled::SampleExtract<Lwe, Rlwe>>::exec(pt_ct, idx);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Torus ref = pt[idx];

    // compute actual result
    Torus res = this->lwe_runtime_.decrypt(res_ct);

    Torus err = res - ref;
    double norm = infinity_norm(err);

    std::cout << "\n=== Sample Extraction Test ===\n";
    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "src " << ": " << pt << "\n";
    }
    std::cout << std::left;
    std::cout << std::setw(14) << "idx " << ": " << idx << "\n";
    std::cout << std::setw(14) << "actual " << ": " << res << "\n";
    std::cout << std::setw(14) << "expected " << ": " << ref << "\n";
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, 0);
  }
}
