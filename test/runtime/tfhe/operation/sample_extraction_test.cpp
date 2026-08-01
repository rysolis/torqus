#include "tfhe/operation/sample_extraction.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace sample_extraction_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename LWE, typename GLWE>
struct ParameterSet {
  using lwe_params = LWE;
  using rlwe_params = GLWE;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 4>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 4>>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 1024>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace sample_extraction_test

template <typename Ctx>
class SampleExtractionFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Lwe = Ctx::lwe_params;
  using Rlwe = Ctx::rlwe_params;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  Executor<Cryptor<Lwe>> tlwe_exe_;
  Executor<Cryptor<Rlwe>> trlwe_exe_;

  void SetUp() override {
    SecretHolder<Rlwe> glwe_kr(this->eng_);
    SecretHolder<Lwe> lwe_kr(glwe_kr.begin(), glwe_kr.end());

    Cryptor<Lwe> tlwe_cryptor(lwe_kr.secret_ptr(), eng_);
    Cryptor<Rlwe> trlwe_cryptor(glwe_kr.secret_ptr(), eng_);

    tlwe_exe_ = Executor<Cryptor<Lwe>>(tlwe_cryptor);
    trlwe_exe_ = Executor<Cryptor<Rlwe>>(trlwe_cryptor);
  }
};

template <typename Config>
class SampleExtractionCorrectnessTest
    : public SampleExtractionFixture<typename Config::context> {
 protected:
  using Base = SampleExtractionFixture<typename Config::context>;

  using rTorus = typename Base::rTorus;
  static constexpr uint32_t N = Base::N;

  struct TestCase {
    size_t idx = 0;
    Poly<rTorus, N> pt;
  };

  void SetUp() override { Base::SetUp(); }

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.idx = 0;
      randomize(tc.pt, this->eng_);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.idx = N / 2;
      randomize(tc.pt, this->eng_);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.idx = N - 1;
      randomize(tc.pt, this->eng_);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(SampleExtractionCorrectnessTest,
                 sample_extraction_test::TestContexts);

TYPED_TEST(SampleExtractionCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  static_assert(n == N, "n and N must be equal for this test");

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    size_t idx = tc.idx;
    Poly<rTorus, N> pt = tc.pt;

    TRLWE<rTorus, N> pt_ct = this->trlwe_exe_.encrypt(pt);

    // ==================================
    // Act
    // ==================================
    TLWE<Torus, n> res_ct =
        Evaluator<SampleExtraction<Lwe, Rlwe>>::exec(pt_ct, idx);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Torus ref = pt[idx];

    // compute actual result
    Torus res = this->tlwe_exe_.decrypt(res_ct);

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
