#include "tfhe/structure/gadget/gadget_repr.hpp"
#include <gtest/gtest.h>

#include <bit>
#include <random>

#include "primitive/torus.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility/utility.hpp"

#include "tfhe/params.hpp"

namespace gadget_repr_test {

template <typename Ctx, bool Verbose = true>
struct TestConfig {
  using context = Ctx;
  static constexpr bool verbose = Verbose;
};

template <typename Rlwe, typename Decomp>
struct ParameterSet {
  using rlwe_params = Rlwe;
  using dcp_params = Decomp;
};

using Ctx1 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;
using Ctx2 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<16>, 4>>,
                          dcp_params<4, 3>>;
using Ctx3 = ParameterSet<rlwe_params<trlwe_core_params<ModTorus<32>, 1024>>,
                          dcp_params<2, 4>>;

using TestContexts = ::testing::Types<TestConfig<Ctx1>, TestConfig<Ctx2>,
                                      TestConfig<Ctx3, false>>;

}  // namespace gadget_repr_test

template <typename Ctx>
class GadgetReprTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using rlwe_params = typename Ctx::context::rlwe_params;
  using Torus = typename rlwe_params::torus_type;
  static constexpr uint32_t N = rlwe_params::N;

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
  using Rlwe = typename TypeParam::context::rlwe_params;
  using Decomp = typename TypeParam::context::dcp_params;

  using Torus = typename Rlwe::torus_type;
  constexpr uint32_t N = Rlwe::N;

  Poly<Torus, N> poly = this->plaintext_;

  EXPECT_EQ(N, poly.size());

  GadgetRepr<Rlwe, Decomp> repr(poly);
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
  std::cout << "  Threshold: " << GadgetRepr<Rlwe, Decomp>::template threshold<Torus>
  << "\n"; std::cout <<
  "=====================================================\n\n";
  // clang-format on

  EXPECT_LT(error_norm, (GadgetRepr<Rlwe, Decomp>::template threshold<Torus>));
}