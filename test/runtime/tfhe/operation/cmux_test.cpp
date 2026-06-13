#include "tfhe/operation/cmux.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"
#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace cmux_test {

struct Ctx1 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr uint32_t l = 3;
};

struct Ctx2 {
  static constexpr bool verbose = false;
  using Torus = ModTorus<32>;
  static constexpr uint32_t N = 1024;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr uint32_t l = 7;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2>;

}  // namespace cmux_test

template <typename Ctx>
class CMuxFixture : public ::testing::Test {
 protected:
  std::mt19937 eng_{0};
  using Torus = typename Ctx::Torus;

  static inline std::shared_ptr<const Poly<UInt, Ctx::N>> secret_;
  std::unique_ptr<trlwe::Cryptor<Ctx>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<Ctx>> trgsw_cryptor_;

  Poly<UInt, Ctx::N> zero_;
  Poly<UInt, Ctx::N> one_;
  TRGSW<Torus, Ctx::N> c0_;
  TRGSW<Torus, Ctx::N> c1_;

  Poly<Torus, Ctx::N> p0_;
  Poly<Torus, Ctx::N> p1_;

  CMux<Ctx> cmux_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->p0_ = Poly<Torus, Ctx::N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });
    this->p1_ = Poly<Torus, Ctx::N>([&eng = this->eng_, &dist = torus_dist]() {
      return static_cast<Torus>(dist(eng));
    });

    // secret
    this->secret_ = std::make_shared<const Poly<UInt, Ctx::N>>(
        [&eng = this->eng_, &dist = binary_dist]() {
          return static_cast<UInt>(dist(eng));
        });

    this->trlwe_cryptor_ =
        std::make_unique<trlwe::Cryptor<Ctx>>(this->secret_, this->eng_);

    this->trgsw_cryptor_ =
        std::make_unique<trgsw::Cryptor<Ctx>>(this->secret_, this->eng_);

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
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TRLWE<Torus, Ctx::N> ep0 =
      this->trlwe_cryptor_->template encrypt<Torus>(this->p0_);

  TRLWE<Torus, Ctx::N> ep1 =
      this->trlwe_cryptor_->template encrypt<Torus>(this->p1_);

  // ==================================
  TRLWE<Torus, Ctx::N> hom_cmux_p0 = this->cmux_(this->c0_, ep0, ep1);
  Poly<Torus, Ctx::N> dp0 =
      this->trlwe_cryptor_->template decrypt<Torus>(hom_cmux_p0);
  // ----------------------------------
  TRLWE<Torus, Ctx::N> hom_cmux_p1 = this->cmux_(this->c1_, ep0, ep1);
  Poly<Torus, Ctx::N> dp1 =
      this->trlwe_cryptor_->template decrypt<Torus>(hom_cmux_p1);
  // ==================================

  double nr0 = infinity_norm(dp0 - this->p0_);
  double nr1 = infinity_norm(dp1 - this->p1_);

  std::cout << "\n=== CMux Test ===\n";
  if (Ctx::verbose) {
    std::cout << "secret    : " << *(this->secret_) << "\n";
    std::cout << "p0: " << this->p0_ << "\n";
    std::cout << "p1: " << this->p1_ << "\n";

    std::cout << "dp0 : " << dp0 << "\n";
    std::cout << "dp1 : " << dp1 << "\n";
  }
  std::cout << "infinity_norm(p0 - dp0): " << nr0 << "\n";
  std::cout << "infinity_norm(p1 - dp1): " << nr1 << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(nr0, 0.1);
  EXPECT_LE(nr1, 0.1);
}