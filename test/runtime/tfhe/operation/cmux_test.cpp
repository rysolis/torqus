#include "tfhe/operation/cmux.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace cmux_test {

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
    glwe_params<trlwe_core_params<ModTorus<32>, 1024>, gadget_params<256, 2>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace cmux_test

template <typename Ctx>
class CMuxFixture : public ::testing::Test {
 protected:
  std::mt19937 eng_{0};

  using params = typename Ctx::context::params;

  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  TrackedCryptor<Cryptor<params>> cryptor_;

  void SetUp() override {
    SecretHolder<params> kr(this->eng_);
    cryptor_ = TrackedCryptor<Cryptor<params>>(kr.secret_ptr(), eng_);
  }
};

template <typename Ctx>
class CMuxCorrectnessTest : public CMuxFixture<Ctx> {
 protected:
  using Base = CMuxFixture<Ctx>;

  using typename Base::params;
  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  TRGSW<Torus, N, l> zero_ct_;
  TRGSW<Torus, N, l> one_ct_;

  Poly<Torus, N> cand0_pt_;
  TRLWE<Torus, N> cand0_ct_;

  Poly<Torus, N> cand1_pt_;
  TRLWE<Torus, N> cand1_ct_;

  void SetUp() override {
    Base::SetUp();

    // Prepare TRGSW(0) and TRGSW(1)
    Poly<UInt, N> zero_pt_, one_pt_;
    zero_pt_[0] = UInt(0);
    one_pt_[0] = UInt(1);

    zero_ct_ = this->cryptor_.encrypt(zero_pt_);
    one_ct_ = this->cryptor_.encrypt(one_pt_);

    // Prepare candidate cand0 and cand1
    randomize(cand0_pt_, this->eng_);
    randomize(cand1_pt_, this->eng_);
    cand0_ct_ = this->cryptor_.encrypt(this->cand0_pt_);
    cand1_ct_ = this->cryptor_.encrypt(this->cand1_pt_);
  }
};

TYPED_TEST_SUITE(CMuxCorrectnessTest, cmux_test::TestContexts);

TYPED_TEST(CMuxCorrectnessTest, SelectorZeroCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  // ==================================
  // Reference
  // ==================================
  Poly<Torus, N> ref_pt = this->cand0_pt_;

  // ==================================
  // TEST LOGIC
  // ==================================
  TRLWE<Torus, N> res_ct = TrackedEvaluator<CMux<params>>::exec(
      this->zero_ct_, this->cand0_ct_, this->cand1_ct_);
  Poly<Torus, N> res_pt = this->cryptor_.decrypt(res_ct);

  // ==================================

  double norm = infinity_norm(res_pt - ref_pt);

  std::cout << "\n=== CMux Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "expected" << ": " << ref_pt << "\n";

    std::cout << std::setw(14) << "actual" << ": " << res_pt << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "norm " << ": " << norm << "\n";
  std::cout << std::setw(14) << "bound" << ": "
            << get_noise_tracker_if()->get(res_ct) << "\n";
  std::cout << "===================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}

TYPED_TEST(CMuxCorrectnessTest, SelectorOneCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  // ==================================
  // Reference
  // ==================================
  Poly<Torus, N> ref_pt = this->cand1_pt_;

  // ==================================
  // TEST LOGIC
  // ==================================
  TRLWE<Torus, N> res_ct = TrackedEvaluator<CMux<params>>::exec(
      this->one_ct_, this->cand0_ct_, this->cand1_ct_);
  Poly<Torus, N> res_pt = this->cryptor_.decrypt(res_ct);

  // ----------------------------------
  double norm = infinity_norm(res_pt - ref_pt);

  std::cout << "\n=== CMux Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "expected" << ": " << ref_pt << "\n";

    std::cout << std::setw(14) << "actual" << ": " << res_pt << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "norm " << ": " << norm << "\n";
  std::cout << std::setw(14) << "bound" << ": "
            << get_noise_tracker_if()->get(res_ct) << "\n";
  std::cout << "===================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}