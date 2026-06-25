#include "tfhe/operation/cmux.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

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

  static inline std::shared_ptr<const Poly<UInt, N>> secret_;
  std::unique_ptr<trlwe::Cryptor<params>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<params>> trgsw_cryptor_;

  Poly<UInt, N> zero_;
  Poly<UInt, N> one_;
  TRGSW<Torus, N> c0_;
  TRGSW<Torus, N> c1_;

  Poly<Torus, N> p0_;
  Poly<Torus, N> p1_;

  CMux<params> cmux_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->p0_ = Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });
    this->p1_ = Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });

    // secret
    this->secret_ = std::make_shared<const Poly<UInt, N>>(
        [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->trlwe_cryptor_ =
        std::make_unique<trlwe::Cryptor<params>>(this->secret_, this->eng_);

    this->trgsw_cryptor_ =
        std::make_unique<trgsw::Cryptor<params>>(this->secret_, this->eng_);

    this->zero_[0] = UInt(0);
    this->one_[0] = UInt(1);

    this->c0_ = trgsw_cryptor_->template encrypt<Torus>(this->zero_);
    this->c1_ = trgsw_cryptor_->template encrypt<Torus>(this->one_);
  }
};

template <typename Ctx>
class CMuxCorrectnessTest : public CMuxFixture<Ctx> {};

TYPED_TEST_SUITE(CMuxCorrectnessTest, cmux_test::TestContexts);

TYPED_TEST(CMuxCorrectnessTest, VerifyCorrectness) {
  using params = typename TypeParam::context::params;

  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  TRLWE<Torus, N> ep0 =
      this->trlwe_cryptor_->template encrypt<Torus>(this->p0_);

  TRLWE<Torus, N> ep1 =
      this->trlwe_cryptor_->template encrypt<Torus>(this->p1_);

  // ==================================
  TRLWE<Torus, N> sut0 = this->cmux_(this->c0_, ep0, ep1);
  Poly<Torus, N> dp0 = this->trlwe_cryptor_->template decrypt<Torus>(sut0);
  // ----------------------------------
  TRLWE<Torus, N> sut1 = this->cmux_(this->c1_, ep0, ep1);
  Poly<Torus, N> dp1 = this->trlwe_cryptor_->template decrypt<Torus>(sut1);
  // ==================================

  double nr0 = infinity_norm(dp0 - this->p0_);
  double nr1 = infinity_norm(dp1 - this->p1_);

  std::cout << "\n=== CMux Test ===\n";
  if (TypeParam::verbose) {
    std::cout << "secret    : " << *(this->secret_) << "\n";
    std::cout << "p0: " << this->p0_ << "\n";
    std::cout << "p1: " << this->p1_ << "\n";

    std::cout << "dp0 : " << dp0 << "\n";
    std::cout << "dp1 : " << dp1 << "\n";
  }
  std::cout << "nr0:              " << nr0 << "\n";
  std::cout << "sut0 error_bound: " << sut0.error_bound() << "\n";

  std::cout << "nr1:              " << nr1 << "\n";
  std::cout << "sut1 error_bound: " << sut1.error_bound() << "\n";
  std::cout << "===================\n\n";

  EXPECT_LE(nr0, sut0.error_bound());
  EXPECT_LE(nr1, sut1.error_bound());
}