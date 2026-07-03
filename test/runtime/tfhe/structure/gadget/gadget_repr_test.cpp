#include "tfhe/structure/gadget/gadget_repr.hpp"
#include <gtest/gtest.h>

#include <bit>
#include <random>

#include "primitive/torus.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

#include "tfhe/params.hpp"

namespace gadget_repr_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename T>
struct ParameterSet {
  using params = T;
};

using Ctx1 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<16>, 4>, gadget_params<4, 3>>>;
using Ctx2 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<16>, 4>, gadget_params<4, 3>>>;
using Ctx3 = ParameterSet<
    glwe_params<trlwe_core_params<ModTorus<32>, 1024>, gadget_params<2, 4>>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>,
                                      TestConfig<Ctx3, false>>;

}  // namespace gadget_repr_test

template <typename Ctx>
class GadgetReprTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using params = typename Ctx::context::params;
  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  // ============================================================
  // test inputs
  // ============================================================

  Poly<Torus, N> plaintext_;

  void SetUp() override {
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->plaintext_ =
        Poly<Torus, N>([&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<Torus>(dist(eng));
        });
  }
};

TYPED_TEST_SUITE(GadgetReprTest, gadget_repr_test::TestContexts);

TYPED_TEST(GadgetReprTest, ReconstructsOriginalPolynomial) {
  using params = typename TypeParam::context::params;
  using Torus = typename params::torus_type;
  constexpr uint32_t N = params::N;

  Poly<Torus, N> poly = this->plaintext_;

  EXPECT_EQ(N, poly.size());

  GadgetRepr<params> repr(poly);
  Poly<Torus, N> reconstructed = repr.template reconstruct<Torus>();

  double error_norm = infinity_norm(poly - reconstructed);

  // clang-format off
  std::cout << "\n=== GadgetRepr Decomposition & Reconstruction Test ===\n";
  if (TypeParam::verbose) {
  std::cout << "Original:      " << poly << "\n";
  std::cout << "Reconstructed: " << reconstructed << "\n";
  }

  std::cout << "Analysis:\n";
  std::cout << "  Infinity norm: " << error_norm << "\n";
  std::cout << "  Threshold: " << GadgetRepr<params>::template threshold<Torus>
  << "\n"; std::cout <<
  "=====================================================\n\n";
  // clang-format on

  EXPECT_LT(error_norm, (GadgetRepr<params>::template threshold<Torus>));
}