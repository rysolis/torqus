#include "tfhe/operation/sample_extraction.hpp"
#include <gtest/gtest.h>

#include <memory>

#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
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
  using glwe_params = GLWE;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 4>>,
                          glwe_params<trlwe_core_params<ModTorus<32>, 4>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>>;

}  // namespace sample_extraction_test

template <typename Ctx>
class SampleExtractionFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using lwe_params = Ctx::lwe_params;
  using glwe_params = Ctx::glwe_params;

  using fTorus = typename lwe_params::torus_type;
  static constexpr uint32_t n = lwe_params::n;

  using bTorus = typename glwe_params::torus_type;
  static constexpr uint32_t N = glwe_params::N;

  Poly<bTorus, N> pt_;
  TRLWE<bTorus, N> pt_ct_;

  TrackedCryptor<Cryptor<lwe_params>> tlwe_cryptor_;
  TrackedCryptor<Cryptor<glwe_params>> trlwe_cryptor_;

  void SetUp() override {
    SecretHolder<glwe_params> glwe_kr(this->eng_);
    SecretHolder<lwe_params> lwe_kr(glwe_kr.begin(), glwe_kr.end());
    tlwe_cryptor_ =
        TrackedCryptor<Cryptor<lwe_params>>(lwe_kr.secret_ptr(), eng_);
    trlwe_cryptor_ =
        TrackedCryptor<Cryptor<glwe_params>>(glwe_kr.secret_ptr(), eng_);

    // Prepare plaintext and its ciphertext
    this->pt_[0] = bTorus(1);
    randomize(this->pt_, eng_);
    this->pt_ct_ = trlwe_cryptor_.encrypt(this->pt_);
  }
};

template <typename Config>
class SampleExtractionCorrectnessTest
    : public SampleExtractionFixture<typename Config::context> {};

TYPED_TEST_SUITE(SampleExtractionCorrectnessTest,
                 sample_extraction_test::TestContexts);

TYPED_TEST(SampleExtractionCorrectnessTest, VerifyCorrectness) {
  using lwe_params = typename TypeParam::context::lwe_params;
  using glwe_params = typename TypeParam::context::glwe_params;

  using fTorus = lwe_params::torus_type;
  constexpr uint32_t n = lwe_params::n;

  // ==================================
  // Reference
  // ==================================
  fTorus ref_pt = this->pt_[0];

  // ==================================
  // TEST LOGIC
  // ==================================
  TLWE<fTorus, n> res_ct =
      SampleExtraction<lwe_params, glwe_params>::exec(this->pt_ct_, 0);
  fTorus res_pt = this->tlwe_cryptor_.decrypt(res_ct);

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
