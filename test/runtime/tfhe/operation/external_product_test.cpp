#include "tfhe/operation/external_product.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"
#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"
#include "tfhe/traits.hpp"

namespace external_product_test {

template <typename Params, bool Verbose = true>
struct Context {
  static constexpr bool verbose = Verbose;
  using params = Params;
};

namespace case1 {
struct Params : public backend_tag {
  struct core {
    using Torus = ModTorus<16>;
    static constexpr uint32_t N = 4;
  };

  struct gadget {
    static constexpr uint32_t B = 4;
    static constexpr uint32_t l = 3;
  };
};
}  // namespace case1

namespace case2 {
struct Params : public backend_tag {
  struct core {
    using Torus = ModTorus<16>;
    static constexpr uint32_t N = 4;
  };

  struct gadget {
    static constexpr uint32_t B = 4;
    static constexpr uint32_t l = 3;
  };
};
}  // namespace case2

namespace case3 {
struct Params : public backend_tag {
  struct core {
    using Torus = ModTorus<16>;
    static constexpr uint32_t N = 8;
  };
  struct gadget {
    static constexpr uint32_t B = 8;
    static constexpr uint32_t l = 3;
  };
};
}  // namespace case3

namespace case4 {
struct Params : public backend_tag {
  struct core {
    using Torus = ModTorus<32>;
    static constexpr uint32_t N = 1024;
  };
  struct gadget {
    static constexpr uint32_t B = 4;
    static constexpr uint32_t l = 7;
  };
};
}  // namespace case4

using Ctx1 = Context<case1::Params>;
using Ctx2 = Context<case2::Params>;
using Ctx3 = Context<case3::Params>;
using Ctx4 = Context<case4::Params, false>;

using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3, Ctx4>;

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using params = traits<typename Ctx::params>;

  using Torus = typename params::Torus;
  static constexpr uint32_t N = params::N;

  static inline std::shared_ptr<const Poly<UInt, N>> secret_;
  std::unique_ptr<trlwe::Cryptor<params>> trlwe_cryptor_;
  std::unique_ptr<trgsw::Cryptor<params>> trgsw_cryptor_;

  // ============================================================
  // test inputs
  // ============================================================

  Poly<UInt, N> multiplier_;
  TRGSW<Torus, N> encrypted_multiplier_;

  Poly<Torus, N> plaintext_;

  ExternalProduct<params> extprod_;

  void SetUp() override {
    std::uniform_int_distribution<UInt::raw_value_type> binary_dist{0, 1};
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->plaintext_ =
        Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
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

    this->multiplier_[0] = UInt(1);

    this->encrypted_multiplier_ =
        trgsw_cryptor_->template encrypt<Torus>(this->multiplier_);
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {};

TYPED_TEST_SUITE(ExternalProductCorrectnessTest,
                 external_product_test::TestContexts);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using Ctx = TypeParam;
  using params = traits<typename Ctx::params>;

  using Torus = typename params::Torus;
  static constexpr uint32_t N = params::N;

  TRLWE<Torus, N> encrypted =
      this->trlwe_cryptor_->template encrypt<Torus>(this->plaintext_);

  // ==================================
  Poly<Torus, N> expected =
      negacyclic_convolution(this->multiplier_, this->plaintext_);
  // ----------------------------------
  TRLWE<Torus, N> hom_mul =
      this->extprod_(this->encrypted_multiplier_, encrypted);
  Poly<Torus, N> decrypted =
      this->trlwe_cryptor_->template decrypt<Torus>(hom_mul);
  // ==================================

  double norm = infinity_norm(decrypted - expected);

  std::cout << "\n=== External Product Test ===\n";
  if (Ctx::verbose) {
    std::cout << "secret    : " << *(this->secret_) << "\n";
    std::cout << "plaintext : " << this->plaintext_ << "\n";
    std::cout << "multiplier: " << this->multiplier_ << "\n";

    std::cout << "decrypted : " << decrypted << "\n";
    std::cout << "expected  : " << expected << "\n";
  }
  std::cout << "infinity_norm: " << norm << "\n";
  std::cout << "===============================\n\n";

  EXPECT_LE(norm, ExternalProduct<params>::threshold);
}
