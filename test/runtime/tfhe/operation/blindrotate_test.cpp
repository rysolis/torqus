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

#include "tfhe/cryptor.hpp"
#include "tfhe/operation/evaluator.hpp"
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

  Poly<Torus, N> tv_;
  TRLWE<Torus, N> tv_ct_;

  BootstrapKey<Torus, N, l, n> BK_;

  TrackedCryptor<Cryptor<Rlwe>> cryptor_;
  std::optional<SecretHolder<Lwe>> lwe_kr;

  void SetUp() override {
    SecretHolder<Rlwe> glwe_kr(this->eng_);
    cryptor_ = TrackedCryptor<Cryptor<Rlwe>>(glwe_kr.secret_ptr(), eng_);

    // Prepare TestVector
    tv_ = testvector::generate<Torus, N>();
    tv_ct_ = cryptor_.encrypt(this->tv_);

    // Prepare SecretHolder
    lwe_kr = std::move(SecretHolder<Lwe>(eng_));

    // Prepare Bootstrapkey
    BK_ = bootstrap_key::generate<Lwe, Rlwe, Dcp>(cryptor_, *lwe_kr);
  }
};

template <typename Config>
class BlindRotateCorrectnessTest
    : public BlindRotateFixture<typename Config::context> {};

TYPED_TEST_SUITE(BlindRotateCorrectnessTest, blindrotate_test::TestContexts);

TYPED_TEST(BlindRotateCorrectnessTest, VerifyCorrectness) {
  using Lwe = typename TypeParam::context::lwe_params;
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  constexpr uint32_t n = Lwe::n;

  using Torus = Rlwe::torus_type;

  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t M = 2 * N;

  ModInt<M> phase(10);

  // ==================================
  // Reference
  // ==================================
  Poly<Torus, N> ref_pt = rotate(this->tv_, (-phase).value());

  // ==================================
  // TEST LOGIC
  // ==================================

  // Prepare Vector<ModInt<M>, n+1>
  Vector<ModInt<M>, n + 1> phase_ct;
  randomize(phase_ct, this->eng_);

  ModInt<M> b{};
  for (size_t i = 0; i < n; ++i) {
    b += static_cast<UInt>(this->lwe_kr->secret()[i]) *
         static_cast<ModInt<M>>(phase_ct[i]);
  }
  b += phase;
  phase_ct[n] = b;  // Overwrite

  // BlindRotate
  TRLWE<Torus, N> res_ct =
      Evaluator<BlindRotate<Lwe, Rlwe, Dcp>, Tracking>::exec(
          this->tv_ct_, phase_ct, this->BK_);
  Poly<Torus, N> res_pt = this->cryptor_.decrypt(res_ct);

  // ----------------------------------
  Poly<Torus, N> err = ref_pt - res_pt;
  double norm = infinity_norm(err);

  std::cout << "\n========================================\n";
  std::cout << "           BlindRotate Test\n";
  std::cout << "========================================\n";

  if (TypeParam::verbose) {
    std::cout << std::left;
    std::cout << std::setw(14) << "tv" << ": " << this->tv_ << '\n';
    std::cout << std::setw(14) << "expected" << ": " << ref_pt << '\n';
    std::cout << std::setw(14) << "actual" << ": " << res_pt << '\n';
  }

  std::cout << std::left;
  std::cout << std::setw(14) << "phase " << ": " << phase << '\n';
  std::cout << std::setw(14) << "-phase" << ": " << -phase << '\n';
  std::cout << std::setw(14) << "norm         " << ": " << norm << '\n';
  std::cout << std::setw(14) << "errror_bound " << ": "
            << get_noise_tracker_if()->get(res_ct) << '\n';

  std::cout << "========================================\n\n";

  EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
}