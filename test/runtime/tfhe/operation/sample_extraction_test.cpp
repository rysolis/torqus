#include "tfhe/operation/sample_extraction.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
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

using TestContexts = ::testing::Types<TestConfig<Ctx1>>;

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

  Poly<rTorus, N> pt_;
  TRLWE<rTorus, N> pt_ct_;

  std::optional<Cryptor<Lwe, Tracking>> tlwe_cryptor_;
  std::optional<Cryptor<Rlwe, Tracking>> trlwe_cryptor_;

  void SetUp() override {
    SecretHolder<Rlwe> glwe_kr(this->eng_);
    SecretHolder<Lwe> lwe_kr(glwe_kr.begin(), glwe_kr.end());
    tlwe_cryptor_ = Cryptor<Lwe, Tracking>(lwe_kr.secret_ptr(), eng_);
    trlwe_cryptor_ = Cryptor<Rlwe, Tracking>(glwe_kr.secret_ptr(), eng_);

    // Prepare plaintext and its ciphertext
    this->pt_[0] = rTorus(1);
    randomize(this->pt_, eng_);
    this->pt_ct_ = trlwe_cryptor_->encrypt(this->pt_);
  }
};

template <typename Config>
class SampleExtractionCorrectnessTest
    : public SampleExtractionFixture<typename Config::context> {};

TYPED_TEST_SUITE(SampleExtractionCorrectnessTest,
                 sample_extraction_test::TestContexts);

TYPED_TEST(SampleExtractionCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  // ==================================
  // Reference
  // ==================================
  Torus ref_pt = this->pt_[0];

  // ==================================
  // TEST LOGIC
  // ==================================
  TLWE<Torus, n> res_ct = Evaluator<SampleExtraction<Lwe, Rlwe>>::exec(
      this->pt_ct_, std::size_t{0});
  Torus res_pt = this->tlwe_cryptor_->decrypt(res_ct);

  // ----------------------------------

  std::cout << "\n=== Sample Extraction Test ===\n";
  std::cout << std::left;
  std::cout << std::setw(14) << "message " << ": " << this->pt_ << "\n";
  std::cout << std::setw(14) << "tlwe " << ": " << this->pt_ct_ << "\n";
  std::cout << std::setw(14) << "expected " << ": " << ref_pt << "\n";
  std::cout << std::setw(14) << "actual " << ": " << res_pt << "\n";
  std::cout << "===============================\n\n";

  EXPECT_EQ(ref_pt, res_pt);
}
