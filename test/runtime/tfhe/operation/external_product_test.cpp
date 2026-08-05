#include "tfhe/operation/external_product.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace external_product_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe, typename Dcp>
struct ParameterSet {
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 8>>,
                          dcp_params<8, 3>>;
using Ctx3 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<256, 2>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>,
                                      TestConfig<Ctx3, false>>;

}  // namespace external_product_test

template <typename Ctx>
class ExternalProductFixture : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Rlwe = typename Ctx::context::rlwe_params;
  using Dcp = typename Ctx::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Dcp::l;

  Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> rlwe_runtime;

  void SetUp() override {
    SecretHolder<Rlwe> kr(eng_);
    rlwe_runtime = Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(
        kr.secret_ptr(), eng_);
  }
};

template <typename Ctx>
class ExternalProductCorrectnessTest : public ExternalProductFixture<Ctx> {
 protected:
  using Base = ExternalProductFixture<Ctx>;

  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  struct TestCase {
    UInt lhs;
    Poly<Torus, N> rhs;
  };

  void SetUp() override { Base::SetUp(); }

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.lhs = UInt(0);
      randomize(tc.rhs, this->eng_);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.lhs = UInt(1);
      randomize(tc.rhs, this->eng_);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(ExternalProductCorrectnessTest,
                 external_product_test::TestContexts);

TYPED_TEST(ExternalProductCorrectnessTest, VerifyCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Dcp::l;

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    UInt lhs = tc.lhs;
    Poly<Torus, N> rhs = tc.rhs;

    // mp means multiplier
    Poly<UInt, N> mp;
    mp[0] = lhs;

    TRGSW<Torus, N, l> mp_ct = this->rlwe_runtime.encrypt(mp);
    TRLWE<Torus, N> rhs_ct = this->rlwe_runtime.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TRLWE<Torus, N> res_ct =
        Evaluator<ExternalProduct<Rlwe, Dcp>, Tracking>::exec(mp_ct, rhs_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<Torus, N> ref = negacyclic_convolution(mp, rhs);

    // compute actual result
    Poly<Torus, N> res = this->rlwe_runtime.decrypt(res_ct);

    Poly<Torus, N> err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n=== External Product Test ===\n";
    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
      std::cout << std::setw(14) << "rhs" << ": " << rhs << "\n";
      std::cout << std::setw(14) << "actual" << ": " << res << "\n";
      std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
    }
    std::cout << std::left;
    std::cout << std::setw(14) << "infinity_norm" << ": " << norm << "\n";
    std::cout << std::setw(14) << "error_bound" << ": "
              << get_noise_tracker_if()->get(res_ct) << "\n";
    std::cout << "===============================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}
