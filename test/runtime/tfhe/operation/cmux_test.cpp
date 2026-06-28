#include "tfhe/operation/cmux.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/keyring/keyring.hpp"
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
  static constexpr uint32_t l = params::l;

  std::unique_ptr<trlwe::Cryptor<params>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<params>> trgsw_cryptor_;

  void SetUp() override {
    KeyRing<params> kr(this->eng_);
    this->trlwe_cryptor_ = std::move(kr.trlwe_cryptor(this->eng_));
    this->trgsw_cryptor_ = std::move(kr.trgsw_cryptor(this->eng_));
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

  Poly<UInt, N> zero_;
  Poly<UInt, N> one_;
  TRGSW<Torus, N, l> c0_;
  TRGSW<Torus, N, l> c1_;

  Poly<Torus, N> p0_;
  Poly<Torus, N> p1_;

  CMux<params> cmux_;

  void SetUp() override {
    Base::SetUp();

    // Prepare TRGSW(0) and TRGSW(1)
    zero_[0] = UInt(0);
    one_[0] = UInt(1);

    c0_ = this->trgsw_cryptor_->template encrypt<Torus>(zero_);
    c1_ = this->trgsw_cryptor_->template encrypt<Torus>(one_);

    // Prepare candidate p0 and p1
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    p0_ = Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });
    p1_ = Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });
  }
};

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

  double norm0 = infinity_norm(dp0 - this->p0_);
  double norm1 = infinity_norm(dp1 - this->p1_);

  std::cout << "\n=== CMux Test ===\n";
  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "p0" << ": " << this->p0_ << "\n";
    std::cout << std::setw(14) << "p1" << ": " << this->p1_ << "\n";

    std::cout << std::setw(14) << "dp0" << ": " << dp0 << "\n";
    std::cout << std::setw(14) << "dp1" << ": " << dp1 << "\n";
  }
  std::cout << std::left;
  std::cout << std::setw(14) << "norm0 " << ": " << norm0 << "\n";
  std::cout << std::setw(14) << "sut0 bound" << ": " << sut0.error_bound()
            << "\n";

  std::cout << std::setw(14) << "norm1 " << ": " << norm1 << "\n";
  std::cout << std::setw(14) << "sut1 bound" << ": " << sut1.error_bound()
            << "\n";
  std::cout << "===================\n\n";

  EXPECT_LE(norm0, sut0.error_bound());
  EXPECT_LE(norm1, sut1.error_bound());
}