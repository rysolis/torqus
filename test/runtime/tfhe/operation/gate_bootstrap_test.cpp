#include "tfhe/operation/gate_bootstrap.hpp"
#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"
#include "tfhe/utility/testvector.hpp"

namespace gate_bootstrap_test {

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

using Context1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 1>>,
                              rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                              dcp_params<4, 3>>;

using Context2 =
    ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 20>>,
                 rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                 dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace gate_bootstrap_test

template <typename Context>
class GateBootstrapFixture : public ::testing::Test {
 protected:
  using Lwe = Context::lwe_params;
  using Rlwe = Context::rlwe_params;
  using Dcp = Context::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = typename Lwe::torus_type;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Dcp::l;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  Runtime<Cryptor<Lwe>, Tracking> lwe_runtime_;
  Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> rlwe_runtime;

  BootstrapKey<rTorus, N, l, n> BK_;

  void SetUp() override {
    SecretHolder<Lwe> lwe_kr(eng_);
    lwe_runtime_ = Runtime<Cryptor<Lwe>, Tracking>(lwe_kr.secret_ptr(), eng_);

    SecretHolder<Rlwe> rlwe_kr(eng_);
    rlwe_runtime = Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(
        rlwe_kr.secret_ptr(), eng_);

    // Prepare Bootstrapkey
    BK_ = rlwe_runtime.template generate_bootstrap_key<Lwe, Rlwe, Dcp>(
        lwe_kr.secret());
  }
};

template <typename Config>
class GateBootstrapCorrectnessTest
    : public GateBootstrapFixture<typename Config::context> {
 protected:
  using Base = GateBootstrapFixture<typename Config::context>;

  using Torus = typename Base::Torus;
  using rTorus = typename Base::rTorus;

  struct TestCase {
    rTorus mu = rTorus(1u, 4u);  // Choose 1/4 in Torus as output
    Torus phase;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.phase = Torus(0u)}, {.phase = Torus(1u, 2u)}};
  }
};

TYPED_TEST_SUITE(GateBootstrapCorrectnessTest,
                 gate_bootstrap_test::TestContexts);

TYPED_TEST(GateBootstrapCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;
  constexpr uint32_t n = Lwe::n;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    rTorus mu = tc.mu;
    Torus phase = tc.phase;

    // Prepare TLWE (constains phase)
    TLWE<Torus, n> tlwe_rot = this->lwe_runtime_.encrypt(Torus(0u));
    tlwe_rot.b() = static_cast<Torus>(tlwe_rot.b()) + phase;

    // Prepare TestVector
    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu.value() >> 1u));

    // ==================================
    // Act
    // ==================================
    TLWE<rTorus, N> res_ct =
        Evaluator<GateBootstrap<Lwe, Rlwe, Dcp>, Tracking>::exec(
            mu, tv, tlwe_rot, this->BK_);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    constexpr uint32_t Q = [] {
      if constexpr (Torus::qbit == 32) {
        return 0;
      } else {
        return 1 << Torus::qbit;
      }
    }();
    ModInt<M> p = mod_switch<M>(ModInt<Q>(phase.value()));
    Poly<rTorus, N> rot = rotate(tv.b(), (-p).value());
    rTorus ref = static_cast<rTorus>(rot[0]) + rTorus(mu.value() / 2);

    // compute actual result
    rTorus res = this->rlwe_runtime.decrypt(res_ct);

    rTorus err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n========================================\n";
    std::cout << "           Gate Bootstrap Test\n";
    std::cout << "========================================\n";

    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "tv" << ": " << tv << '\n';
    }

    std::cout << std::left;
    std::cout << std::setw(14) << "mu" << ": " << mu << " (" << double(mu)
              << ")\n";
    std::cout << std::setw(14) << "phase" << ": " << phase << '\n';
    std::cout << std::setw(14) << "phase(mod)" << ": " << p << '\n';
    std::cout << std::setw(14) << "expected" << ": " << ref << " ("
              << double(ref) << ")\n";
    std::cout << std::setw(14) << "actual" << ": " << res << " (" << double(res)
              << ")\n";
    std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
    std::cout << std::setw(14) << "errror_bound " << ": "
              << get_noise_tracker_if()->get(res_ct) << '\n';

    std::cout << "========================================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}