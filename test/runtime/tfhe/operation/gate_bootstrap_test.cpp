#include "tfhe/operation/gate_bootstrap.hpp"
#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"
#include "algebra/vector.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"
#include "tfhe/utility/testvector.hpp"

namespace gate_bootstrap_test {

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

using Ctx1 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<16>, 1>>,
                          rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;

using Ctx2 = ParameterSet<lwe_params<tlwe_core_params<ModTorus<32>, 20>>,
                          rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace gate_bootstrap_test

template <typename Ctx>
class GateBootstrapFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{10};
  using Lwe = Ctx::lwe_params;
  using Rlwe = Ctx::rlwe_params;
  using Dcp = Ctx::dcp_params;

  static constexpr uint32_t n = Lwe::n;

  using Torus = Lwe::torus_type;
  using rTorus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Dcp::l;

  TLWE<Torus, n> tlwe_rot_;

  Executor<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> exe_;
  Executor<Cryptor<Lwe>, Tracking> tlwe_exe_;

  BootstrapKey<rTorus, N, l, n> BK_;

  void SetUp() override {
    SecretHolder<Rlwe> glwe_kr(this->eng_);
    Cryptor<ParamsPack<Rlwe, Dcp>> cryptor(glwe_kr.secret_ptr(), eng_);
    exe_ = Executor<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(cryptor);

    SecretHolder<Lwe> lwe_kr(eng_);
    Cryptor<Lwe> lwe_cryptor(lwe_kr.secret_ptr(), eng_);
    tlwe_exe_ = Executor<Cryptor<Lwe>, Tracking>(lwe_cryptor);

    // Prepare TLWE
    tlwe_rot_ = tlwe_exe_.encrypt(Torus(0u));

    // Prepare Bootstrapkey
    BK_ = bootstrap_key::generate<Lwe, Rlwe, Dcp>(exe_, lwe_kr);
  }
};

template <typename Config>
class GateBootstrapCorrectnessTest
    : public GateBootstrapFixture<typename Config::context> {
 protected:
  using Base = GateBootstrapFixture<typename Config::context>;

  using Torus = Base::Torus;
  using rTorus = Base::rTorus;
  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  static constexpr uint32_t M = 2 * N;

  struct TestCase {
    // Choose 1/4 in Torus as output
    rTorus mu = rTorus(1u, 4u);
    Torus phase;
  };

  void SetUp() override { Base::SetUp(); }

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.phase = Torus(0u);  // encode 0 in Torus
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.phase = Torus(1u, 2u);  // encode 1/2 in Torus
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(GateBootstrapCorrectnessTest,
                 gate_bootstrap_test::TestContexts);

TYPED_TEST(GateBootstrapCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = Lwe::torus_type;

  using rTorus = Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    rTorus mu = tc.mu;
    Torus phase = tc.phase;

    // Prepare TLWE (constains phase)
    this->tlwe_rot_.b() = static_cast<Torus>(this->tlwe_rot_.b()) + phase;

    // Prepare TestVector
    Poly<rTorus, N> tv =
        testvector::generate<rTorus, N>(rTorus(mu.value() >> 1u));
    TRLWE<rTorus, N> tv_ct = this->exe_.encrypt(tv);

    // ==================================
    // Act
    // ==================================
    TLWE<rTorus, N> res_ct =
        Evaluator<GateBootstrap<Lwe, Rlwe, Dcp>, Tracking>::exec(
            mu, tv_ct, this->tlwe_rot_, this->BK_);

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
    Poly<rTorus, N> rot = rotate(tv, (-p).value());
    rTorus ref = static_cast<rTorus>(rot[0]) + rTorus(mu.value() >> 1u);

    // compute actual result
    rTorus res = this->exe_.decrypt(res_ct);

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