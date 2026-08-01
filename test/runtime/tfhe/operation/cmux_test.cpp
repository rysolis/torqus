#include "tfhe/operation/cmux.hpp"
#include <gtest/gtest.h>

#include <memory>
#include <random>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/randomize.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/executor.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace cmux_test {

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
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2, false>>;

}  // namespace cmux_test

template <typename Ctx>
class CMuxFixture : public ::testing::Test {
 protected:
  std::mt19937 eng_{0};

  using Rlwe = typename Ctx::context::rlwe_params;
  using Dcp = typename Ctx::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t l = Dcp::l;

  Executor<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> exe_;

  void SetUp() override {
    SecretHolder<Rlwe> kr_(this->eng_);
    Cryptor<ParamsPack<Rlwe, Dcp>> cryptor(kr_.secret_ptr(), eng_);
    exe_ = Executor<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(cryptor);
  }
};

template <typename Ctx>
class CMuxCorrectnessTest : public CMuxFixture<Ctx> {
 protected:
  using Base = CMuxFixture<Ctx>;

  using typename Base::Dcp;
  using typename Base::Rlwe;
  using typename Base::Torus;

  static constexpr uint32_t N = Base::N;
  static constexpr uint32_t l = Base::l;

  struct TestCase {
    UInt selector;
    Poly<Torus, N> lhs;
    Poly<Torus, N> rhs;
  };

  void SetUp() override { Base::SetUp(); }

  [[nodiscard]] std::vector<TestCase> cases() {
    std::vector<TestCase> cases;
    {
      TestCase tc;
      tc.selector = UInt(0);
      randomize(tc.lhs, this->eng_);
      randomize(tc.rhs, this->eng_);
      cases.push_back(std::move(tc));
    }
    {
      TestCase tc;
      tc.selector = UInt(1);
      randomize(tc.lhs, this->eng_);
      randomize(tc.rhs, this->eng_);
      cases.push_back(std::move(tc));
    }
    return cases;
  }
};

TYPED_TEST_SUITE(CMuxCorrectnessTest, cmux_test::TestContexts);

TYPED_TEST(CMuxCorrectnessTest, SelectorZeroCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t l = Dcp::l;

  for (const auto& tc : this->cases()) {
    // ==================================
    // Arrange
    // ==================================
    UInt selector = tc.selector;
    Poly<Torus, N> lhs = tc.lhs;
    Poly<Torus, N> rhs = tc.rhs;

    Poly<UInt, N> sl;
    sl[0] = selector;
    TRGSW<Torus, N, l> selector_ct = this->exe_.encrypt(sl);

    TRLWE<Torus, N> lhs_ct = this->exe_.encrypt(lhs);
    TRLWE<Torus, N> rhs_ct = this->exe_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TRLWE<Torus, N> res_ct =
        Evaluator<CMux<Rlwe, Dcp>, Tracking>::exec(selector_ct, lhs_ct, rhs_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<Torus, N> ref = [selector, &lhs, &rhs] {
      if (selector == UInt(0)) {
        return lhs;
      } else {
        return rhs;
      }
    }();

    // compute actual result
    Poly<Torus, N> res = this->exe_.decrypt(res_ct);

    Poly<Torus, N> err = ref - res;
    double norm = infinity_norm(err);

    std::cout << "\n=== CMux Test ===\n";
    if (TypeParam::verbose) {
      std::cout << std::left;
      std::cout << std::setw(14) << "selector" << ": " << selector << "\n";
      std::cout << std::setw(14) << "lhs" << ": " << lhs << "\n";
      std::cout << std::setw(14) << "rhs" << ": " << rhs << "\n";
      std::cout << std::setw(14) << "expected" << ": " << ref << "\n";
      std::cout << std::setw(14) << "actual" << ": " << res << "\n";
    }
    std::cout << std::left;
    std::cout << std::setw(14) << "infinity_norm " << ": " << norm << "\n";
    std::cout << std::setw(14) << "error_bound" << ": "
              << get_noise_tracker_if()->get(res_ct) << "\n";
    std::cout << "===================\n\n";

    EXPECT_LE(norm, get_noise_tracker_if()->get(res_ct));
  }
}