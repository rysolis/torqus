#include "tfhe/structure/gadget_repr.hpp"

#include <gtest/gtest.h>

#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

namespace gadget_repr_test {

struct Ctx1 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx2 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 8;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx3 {
  static constexpr bool verbose = false;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 1024;
  static constexpr uint32_t B = 2;
  static constexpr uint32_t l = 4;
};

}  // namespace gadget_repr_test

using TestContexts =
    ::testing::Types<gadget_repr_test::Ctx1, gadget_repr_test::Ctx2,
                     gadget_repr_test::Ctx3>;

template <typename Ctx>
class GadgetReprTest : public ::testing::Test {};

TYPED_TEST_SUITE(GadgetReprTest, TestContexts);

TYPED_TEST(GadgetReprTest, ReconstructsOriginalPolynomial) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};
  std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
      Torus::raw_min(), Torus::raw_max());

  Poly<Torus> poly(Ctx::N, [&eng = eng_, &dist = torus_dist]() {
    return static_cast<Torus>(dist(eng));
  });

  GadgetRepr<Ctx> repr(poly);

  Poly<Torus> reconstructed_poly = repr.template reconstruct<Torus>();

  double error_norm = infinity_norm(poly - reconstructed_poly);

  // clang-format off
  std::cout << "\n=== GadgetRepr Decomposition & Reconstruction Test ===\n";
  if (Ctx::verbose) {
    std::cout << "Original:      " << poly << "\n";
    std::cout << "Reconstructed: " << reconstructed_poly << "\n";
  }

  std::cout << "Error Analysis:\n";
  std::cout << "  Infinity norm: " << error_norm << "\n";
  std::cout << "  Threshold: " << GadgetRepr<Ctx>::threshold << "\n";
  std::cout << "=====================================================\n\n";
  // clang-format on

  EXPECT_LT(error_norm, (GadgetRepr<Ctx>::threshold));
}