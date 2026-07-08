#include "tfhe/operation/blindrotate.hpp"
#include <gtest/gtest.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <random>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracked.hpp"
#include "tfhe/utility/secret_holder.hpp"
#include "tfhe/utility/testvector.hpp"

namespace blindrotate_test {

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

using Ctx1 = ParameterSet<
    lwe_params<tlwe_core_params<void, 1>>,
    glwe_params<trlwe_core_params<ModTorus<16>, 4>, gadget_params<4, 3>>>;

using Ctx2 = ParameterSet<
    lwe_params<tlwe_core_params<void, 20>>,
    glwe_params<trlwe_core_params<ModTorus<32>, 1024>, gadget_params<256, 2>>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace blindrotate_test

template <typename Ctx>
class BlindRotateFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  using lwe_params = Ctx::lwe_params;
  using glwe_params = Ctx::glwe_params;

  static constexpr uint32_t n = lwe_params::n;

  using bTorus = glwe_params::torus_type;
  static constexpr uint32_t N = glwe_params::N;
  static constexpr uint32_t l = glwe_params::l;
  static constexpr uint32_t M = 2 * N;

  ModInt<M> phase_;
  Poly<bTorus, N> tv_;

  TrackedCryptor<Cryptor<glwe_params>> cryptor_;

  Vector<ModInt<M>, n + 1> mod_tlwe_;
  TRLWE<bTorus, N> trlwe_tv_;
  BootstrapKey<bTorus, N, l, n> bk_;

  void SetUp() override {
    SecretHolder<glwe_params> glwe_kr(this->eng_);
    this->cryptor_ =
        TrackedCryptor<Cryptor<glwe_params>>(glwe_kr.secret_ptr(), eng_);

    SecretHolder<lwe_params> lwe_kr(this->eng_);

    // Prepare Bootstrapkey
    for (size_t i = 0; i < n; ++i) {
      Poly<UInt, N> tmp;
      tmp[0] = static_cast<UInt>((lwe_kr.secret())[i]);
      this->bk_[i] = cryptor_.encrypt(tmp);
    }

    // Prepare Vector<ModInt<M>, n+1>
    std::uniform_int_distribution<typename ModInt<M>::raw_value_type>
        modint_dist(ModInt<M>::raw_min(), ModInt<M>::raw_max());

    this->phase_ = ModInt<M>(10);
    this->mod_tlwe_ =
        Vector<ModInt<M>, n + 1>([&eng = this->eng_, &dist = modint_dist]() {
          return static_cast<ModInt<M>>(dist(eng));
        });

    ModInt<M> b{};
    for (size_t i = 0; i < n; ++i) {
      b += static_cast<UInt>(lwe_kr.secret()[i]) *
           static_cast<ModInt<M>>(this->mod_tlwe_[i]);
    }
    b += this->phase_;
    this->mod_tlwe_[n] = b;  // Overwrite

    // Prepare TestVector
    this->tv_ = testvector::generate<bTorus, N>();
    this->trlwe_tv_ = cryptor_.encrypt(this->tv_);
  }
};

template <typename Config>
class BlindRotateCorrectnessTest
    : public BlindRotateFixture<typename Config::context> {};

TYPED_TEST_SUITE(BlindRotateCorrectnessTest, blindrotate_test::TestContexts);

TYPED_TEST(BlindRotateCorrectnessTest, VerifyCorrectness) {
  using lwe_params = typename TypeParam::context::lwe_params;
  using glwe_params = typename TypeParam::context::glwe_params;

  using bTorus = glwe_params::torus_type;

  constexpr uint32_t N = glwe_params::N;
  constexpr uint32_t M = 2 * N;

  ModInt<M> phase = this->phase_;

  // ==================================
  Poly<bTorus, N> expected = rotate(this->tv_, (-phase).value());
  // ----------------------------------
  TRLWE<bTorus, N> encrypted =
      TrackedEvaluator<BlindRotate<lwe_params, glwe_params>>::exec(
          this->trlwe_tv_, this->mod_tlwe_, this->bk_);
  Poly<bTorus, N> decrypted = this->cryptor_.decrypt(encrypted);
  // ==================================

  Poly<bTorus, N> err = decrypted - expected;
  double norm = infinity_norm(err);

  std::cout << "\n========================================\n";
  std::cout << "           BlindRotate Test\n";
  std::cout << "========================================\n";

  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "tv" << ": " << this->tv_ << '\n';
    std::cout << std::setw(14) << "expected" << ": " << expected << '\n';
    std::cout << std::setw(14) << "decrypted" << ": " << decrypted << '\n';
  }

  std::cout << std::left;
  std::cout << std::setw(14) << "phase " << ": " << phase << '\n';
  std::cout << std::setw(14) << "-phase" << ": " << -phase << '\n';
  std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
  std::cout << std::setw(14) << "errror_bound " << ": "
            << get_noise_tracker_if()->get(encrypted) << '\n';

  std::cout << "========================================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(encrypted));
}