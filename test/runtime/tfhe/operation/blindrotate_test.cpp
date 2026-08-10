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
#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/random_generator.hpp"
#include "tfhe/utility/testvector.hpp"

namespace blindrotate_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Lwe, typename Rlwe, typename Dcp>
struct ParameterSet {
  using lwe_params = Lwe;
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Context1 = ParameterSet<lwe_params<tlwe_core_params<void, 1>>,
                              rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                              dcp_params<4, 3>>;

using Context2 =
    ParameterSet<lwe_params<tlwe_core_params<void, 20>>,
                 rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                 dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace blindrotate_test

template <typename Context>
class BlindRotateFixture : public ::testing::Test {
 protected:
  using Lwe = typename Context::lwe_params;
  using Rlwe = typename Context::rlwe_params;
  using Dcp = typename Context::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Dcp::l;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{10};

  Runtime<Cryptor<Rlwe>> rlwe_runtime_;

  BootstrapKey<rTorus, N, l, n> BK_;
  Vector<ModInt<M>, n + 1> phase_ct_;

  void SetUp() override {
    Runtime<Cryptor<Lwe>> lwe_runtime = Runtime<Cryptor<Lwe>>(eng_);
    rlwe_runtime_ = Runtime<Cryptor<Rlwe>>(eng_);

    // Prepare Bootstrapkey
    BK_ = rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Dcp>(
        lwe_runtime.holder().get());

    double bound = 0.0;  // TODO: use parameters to compute
    get_key_noise_tracker_if()->update(BK_, bound);

    // Prepare Vector<ModInt<M>, n + 1> phase_ct;
    randomize(phase_ct_, this->eng_);

    ModInt<M> b{};
    for (size_t i = 0; i < n; ++i) {
      b += static_cast<UInt>(lwe_runtime.holder().get()[i]) *
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

  using rTorus = typename Base::rTorus;
  static constexpr uint32_t M = Base::M;

  struct TestCase {
    rTorus mu = rTorus(1u, 2u);  // encode 1/2 in rTorus
    ModInt<M> phase;
  };

  [[nodiscard]] static std::vector<TestCase> cases() {
    return {{.phase = ModInt<M>(0)},
            {.phase = ModInt<M>(1)},
            {.phase = ModInt<M>(M - 1)},
            {.phase = ModInt<M>(M / 2)}};
  }
};

TYPED_TEST_SUITE(BlindRotateCorrectnessTest, blindrotate_test::TestContexts);

TYPED_TEST(BlindRotateCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    rTorus mu = tc.mu;
    ModInt<M> phase = tc.phase;

    // Prepare Vector<ModInt<M>, n+1> (contains phase)
    Vector<ModInt<M>, n + 1> phase_ct = this->phase_ct_;
    phase_ct[n] = static_cast<ModInt<M>>(phase_ct[n]) + phase;

    // Prepare TestVector
    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu.value() >> 1u));

    // ==================================
    // Act
    // ==================================
    TRLWE<rTorus, N> res_ct =
        Evaluator<BlindRotate<Lwe, Rlwe, Dcp>, Tracking>::exec(tv, phase_ct,
                                                               this->BK_);
    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = rotate(tv.b(), (-phase).value());

    // compute actual result
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    Poly<rTorus, N> err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n========================================\n";
    std::cout << "           BlindRotate Test\n";
    std::cout << "========================================\n";

    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "tv" << ": " << tv.b() << '\n';
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