#include "tfhe/structure/gadget_repr.hpp"

#include <gtest/gtest.h>

#include <random>

#include "algebra/poly.hpp"
#include "tfhe/params.hpp"

namespace gadget_repr_test {

struct Ctx1 {
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx2 {
  static constexpr uint32_t N = 8;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t l = 3;
};

struct Ctx3 {
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

  std::mt19937 eng{std::random_device{}()};
  std::uniform_int_distribution<ModTorus::raw_value_type> dist(0, Torus::Q - 1);

  Poly<ModTorus> poly(
      Ctx::N, [&eng, &dist]() { return static_cast<ModTorus>(dist(eng)); });

  GadgetRepr<Ctx> repr(poly);

  Poly<ModTorus> reconstructed_poly = repr.reconstruct();

  double error_norm = infinity_norm(poly - reconstructed_poly);

  // clang-format off
  std::cout << "\n=== GadgetRepr Decomposition & Reconstruction Test ===\n";
  // std::cout << "Original poly (Torus):    " << poly << "\n";
  // std::cout << "Original poly (ModTorus): " << poly_mod << "\n";

  // std::cout << "Reconstructed poly (ModTorus): " << reconstructed_poly_mod <<
  // "\n"; std::cout << "Reconstructed poly (Torus): " << reconstructed_poly <<
  // "\n";

  std::cout << "Error Analysis:\n";
  std::cout << "  Infinity norm: " << error_norm << "\n";
  std::cout << "  Threshold: " << GadgetRepr<Ctx>::threshold << "\n";
  std::cout << "=====================================================\n\n";
  // clang-format on

  EXPECT_LT(error_norm, GadgetRepr<Ctx>::threshold);
}