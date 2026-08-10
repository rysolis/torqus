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
#include "tfhe/feature.hpp"
#include "tfhe/operation/evaluator.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/random_generator.hpp"

namespace cmux_test {

template <typename Context, bool Verbose = true>
struct TestConfig {
  using context = Context;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe, typename Dcp>
struct ParameterSet {
  using rlwe_params = Rlwe;
  using dcp_params = Dcp;
};

using Context1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                              dcp_params<4, 3>>;
using Context2 =
    ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                 dcp_params<256, 2>>;

using TestContexts =
    ::testing::Types<TestConfig<Context1>, TestConfig<Context2, false>>;

}  // namespace cmux_test

template <typename Context>
class CMuxFixture : public ::testing::Test {
 protected:
  using Rlwe = typename Context::rlwe_params;
  using Dcp = typename Context::dcp_params;

  using rTorus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  RandomGenerator<std::mt19937> eng_{0};

  Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking> rlwe_runtime_;

  void SetUp() override {
    rlwe_runtime_ = Runtime<Cryptor<ParamsPack<Rlwe, Dcp>>, Tracking>(eng_);
  }
};

template <typename Config>
class CMuxCorrectnessTest : public CMuxFixture<typename Config::context> {
 protected:
  using Base = CMuxFixture<typename Config::context>;

  using rTorus = typename Base::rTorus;
  static constexpr uint32_t N = Base::N;

  struct TestCase {
    UInt selector;
    Poly<rTorus, N> lhs;
    Poly<rTorus, N> rhs;
  };

  [[nodiscard]] std::vector<TestCase> cases() {
    return {{.selector = UInt(0),
             .lhs = randomize<Poly<rTorus, N>>(this->eng_),
             .rhs = randomize<Poly<rTorus, N>>(this->eng_)},
            {.selector = UInt(1),
             .lhs = randomize<Poly<rTorus, N>>(this->eng_),
             .rhs = randomize<Poly<rTorus, N>>(this->eng_)}};
  }
};

TYPED_TEST_SUITE(CMuxCorrectnessTest, cmux_test::TestContexts);

TYPED_TEST(CMuxCorrectnessTest, SelectorZeroCorrectness) {
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Dcp = typename TypeParam::context::dcp_params;

  using rTorus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t l = Dcp::l;

  for (const auto& tc : TestFixture::cases()) {
    // ==================================
    // Arrange
    // ==================================
    UInt selector = tc.selector;
    Poly<rTorus, N> lhs = tc.lhs;
    Poly<rTorus, N> rhs = tc.rhs;

    Poly<UInt, N> sl;
    sl[0] = selector;
    TRGSW<rTorus, N, l> selector_ct = this->rlwe_runtime_.encrypt(sl);

    TRLWE<rTorus, N> lhs_ct = this->rlwe_runtime_.encrypt(lhs);
    TRLWE<rTorus, N> rhs_ct = this->rlwe_runtime_.encrypt(rhs);

    // ==================================
    // Act
    // ==================================
    TRLWE<rTorus, N> res_ct =
        Evaluator<CMux<Rlwe, Dcp>, Tracking>::exec(selector_ct, lhs_ct, rhs_ct);

    // ==================================
    // Assert
    // ==================================
    // compute reference result
    Poly<rTorus, N> ref = [selector, &lhs, &rhs] {
      if (selector == UInt(0)) {
        return lhs;
      } else {
        return rhs;
      }
    }();

    // compute actual result
    Poly<rTorus, N> res = this->rlwe_runtime_.decrypt(res_ct);

    Poly<rTorus, N> err = ref - res;
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