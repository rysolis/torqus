#include "tfhe/operation/external_product.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace external_product_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<16>, 4>, gadget_params<4, 3>>>;
using Ctx2 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<16>, 8>, gadget_params<8, 3>>>;
using Ctx3 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<32>, 1024>, gadget_params<256, 2>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>,
                                      TestConfig<Ctx3, false>>;

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  TrackedCryptor<Cryptor<params>> cryptor_;

  void SetUp() override {
    SecretHolder<params> kr(eng_);
    cryptor_ = TrackedCryptor<Cryptor<params>>(kr.secret_ptr(), eng_);
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {
 protected:
  using Base = ExternalProductFixture<Ctx>;

  using typename Base::params;
  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  Poly<Torus, N> pt_;
  TRLWE<Torus, N> pt_ct_;

  void SetUp() override {
    Base::SetUp();

    // Prepare plaintext and its ciphertext
    randomize(pt_, this->eng_);
    pt_ct_ = this->cryptor_.encrypt(pt_);
  }
};

TYPED_TEST_SUITE(ExternalProductCorrectnessTest,
                 external_product_test::TestContexts);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  // mp means multiplier
  Poly<UInt, N> mp_pt_;
  mp_pt_[0] = UInt(1);

  // ==================================
  // Reference
  // ==================================
  Poly<Torus, N> ref_pt = negacyclic_convolution(mp_pt_, this->pt_);

  // ==================================
  // TEST LOGIC
  // ==================================
  TRGSW<Torus, N, l> mp_ct = this->cryptor_.encrypt(mp_pt_);

  TRLWE<Torus, N> res_ct =
      TrackedEvaluator<ExternalProduct<params>>::exec(mp_ct, this->pt_ct_);
  Poly<Torus, N> res_pt = this->cryptor_.decrypt(res_ct);

  // ----------------------------------
  Poly<Torus, N> err = ref_pt - res_pt;
  double norm = infinity_norm(err);

  std::cout << "\n=== External Product Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "pt" << ": " << this->pt_ << "\n";
    std::cout << std::setw(14) << "mp" << ": " << mp_pt_ << "\n";

    std::cout << std::setw(14) << "expected" << ": " << ref_pt << "\n";
    std::cout << std::setw(14) << "actual" << ": " << res_pt << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
  std::cout << std::setw(14) << "error_bound" << ": "
            << get_noise_tracker_if()->get(res_ct) << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}
