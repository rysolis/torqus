#include "tfhe/operation/blindrotate.hpp"
#include <gtest/gtest.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <random>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"
#include "tfhe/utility/testvector.hpp"

namespace blindrotate_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe, typename Dcp>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<void, 1>>,
                          rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<void, 20>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace blindrotate_test

template <typename Ctx>
class BlindRotateFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{10};
  using Lwe = Ctx::lwe_params;
  using Rlwe = Ctx::rlwe_params;
  using Dcp = Ctx::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Dcp::l;

  Executor<Cryptor<Rlwe>, Tracking> exe_;

  BootstrapKey<Torus, N, l, n> BK_;
  Vector<ModInt<M>, n + 1> phase_ct_;

  void SetUp() override {
    SecretHolder<Rlwe> glwe_kr(this->eng_);
    Cryptor<Rlwe> cryptor(glwe_kr.secret_ptr(), eng_);
    exe_ = Executor<Cryptor<Rlwe>, Tracking>(cryptor);

    // Prepare SecretHolder
    SecretHolder<Lwe> lwe_kr(eng_);

    // Prepare Bootstrapkey
    BK_ = bootstrap_key::generate<Lwe, Rlwe, Dcp>(exe_, lwe_kr);

    // Prepare Vector<ModInt<M>, n + 1> phase_ct;
    randomize(phase_ct_, this->eng_);

    ModInt<M> b{};
    for (size_t i = 0; i < n; ++i) {
      b += static_cast<UInt>(lwe_kr.secret()[i]) *
           static_cast<ModInt<M>>(phase_ct_[i]);
    }

    phase_ct_[n] = b;  // Overwrite!
  }
};

template <typename Config>
class BlindRotateCorrectnessTest
    : public BlindRotateFixture<typename Config::context> {
 protected:
  using Base = BlindRotateFixture<typename Config::context>;

  using Torus = Base::Torus;
  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  static constexpr uint32_t M = 2 * N;

  struct TestCase {
    Torus mu = Torus(1u << (Torus::qbit - 2));
    ModInt<M> phase;
  };

  void SetUp() override { Base::SetUp(); }

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.phase = ModInt<M>(0);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.phase = ModInt<M>(1);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.phase = ModInt<M>(M - 1);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.phase = ModInt<M>(M / 2);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(BlindRotateCorrectnessTest, blindrotate_test::TestContexts);

TYPED_TEST(BlindRotateCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  constexpr uint32_t n = Lwe::n;

  using Torus = Rlwe::torus_type;

  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    Torus mu = tc.mu;
    ModInt<M> phase = tc.phase;

    // Prepare Vector<ModInt<M>, n+1> (contains phase)
    Vector<ModInt<M>, n + 1> phase_ct = this->phase_ct_;
    phase_ct[n] = static_cast<ModInt<M>>(phase_ct[n]) + phase;

    // Prepare TestVector
    Poly<Torus, N> tv = testvector::generate<Torus, N>(Torus(mu.value() >> 1));
    TRLWE<Torus, N> tv_ct = this->exe_.encrypt(tv);

    // ==================================
    // Act
    // ==================================
    TRLWE<Torus, N> res_ct =
        Evaluator<BlindRotate<Lwe, Rlwe, Dcp>, Tracking>::exec(tv_ct, phase_ct,
                                                               this->BK_);
    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<Torus, N> ref = rotate(tv, (-phase).value());

    // compute actual result
    Poly<Torus, N> res = this->exe_.decrypt(res_ct);

    Poly<Torus, N> err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n========================================\n";
    std::cout << "           BlindRotate Test\n";
    std::cout << "========================================\n";

    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "tv" << ": " << tv << '\n';
      std::cout << std::setw(14) << "expected" << ": " << ref << '\n';
      std::cout << std::setw(14) << "actual" << ": " << res << '\n';
    }

    std::cout << std::left;
    std::cout << std::setw(14) << "mu" << ": " << mu << '\n';
    std::cout << std::setw(14) << "phase " << ": " << phase << '\n';
    std::cout << std::setw(14) << "-phase" << ": " << -phase << '\n';
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
    std::cout << std::setw(14) << "errror_bound " << ": "
              << get_noise_tracker_if()->get(res_ct) << '\n';

    std::cout << "========================================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}