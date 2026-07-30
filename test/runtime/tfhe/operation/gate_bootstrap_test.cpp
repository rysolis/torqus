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

  // Choose 1/4 in Torus as output
  rTorus mu_{1u << (rTorus::qbit - 2)};

  Poly<rTorus, N> tv_;
  TRLWE<rTorus, N> tv_ct_;

  Torus rot_;
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

    // Prepare TestVector
    rTorus c = rTorus(mu_.value() / 2);
    tv_ = testvector::generate<rTorus, N>(c);
    tv_ct_ = exe_.encrypt(this->tv_);

    // Prepare TLWE
    Torus phase(1u << (Torus::qbit - 1));  // encode 1/2 in Torus
    tlwe_rot_ = tlwe_exe_.encrypt(phase);

    // Prepare Bootstrapkey
    BK_ = bootstrap_key::generate<Lwe, Rlwe, Dcp>(exe_, lwe_kr);
  }
};

template <typename Config>
class GateBootstrapCorrectnessTest
    : public GateBootstrapFixture<typename Config::context> {};

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

  // ==================================
  // Reference
  // ==================================
  constexpr uint32_t Q = [] {
    if constexpr (Torus::qbit == 32) {
      return 0;
    } else {
      return 1 << Torus::qbit;
    }
  }();
  ModInt<M> p = mod_switch<M>(ModInt<Q>(1u << (Torus::qbit - 1)));
  Poly<rTorus, N> rot = rotate(this->tv_, (-p).value());
  rTorus ref_pt = static_cast<rTorus>(rot[0]) + rTorus(this->mu_.value() / 2);

  // ==================================
  // TEST LOGIC
  // ==================================

  // Gate Bootstrap
  TLWE<rTorus, N> res_ct =
      Evaluator<GateBootstrap<Lwe, Rlwe, Dcp>, Tracking>::exec(
          this->mu_, this->tv_ct_, this->tlwe_rot_, this->BK_);
  rTorus res_pt = this->exe_.decrypt(res_ct);

  // ----------------------------------
  rTorus err = ref_pt - res_pt;
  double norm = infinity_norm(err);

  std::cout << "\n========================================\n";
  std::cout << "           Gate Bootstrap Test\n";
  std::cout << "========================================\n";

  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "tv" << ": " << this->tv_ << '\n';
    std::cout << std::setw(14) << "p" << ": " << p << '\n';
    std::cout << std::setw(14) << "rot" << ": " << rot << '\n';
  }

  std::cout << std::left;
  std::cout << std::setw(14) << "expected" << ": " << ref_pt << '\n';
  std::cout << std::setw(14) << "actual" << ": " << res_pt << '\n';
  std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
  std::cout << std::setw(14) << "errror_bound " << ": "
            << get_noise_tracker_if()->get(res_ct) << '\n';

  std::cout << "========================================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}