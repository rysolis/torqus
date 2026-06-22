#include "tfhe/structure/gadget_repr.hpp"
#include <gtest/gtest.h>

#include <bit>
#include <random>

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

namespace gadget_repr_test {

struct Ctx1 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr uint32_t l = 3;
};

struct Ctx2 {
  static constexpr bool verbose = true;
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 8;
  static constexpr uint32_t B = 4;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr uint32_t l = 3;
};

struct Ctx3 {
  static constexpr bool verbose = false;
  using Torus = ModTorus<32>;
  static constexpr uint32_t N = 1024;
  static constexpr uint32_t B = 2;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr uint32_t l = 4;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3>;

}  // namespace gadget_repr_test

template <typename Ctx>
class GadgetReprTest : public ::testing::Test {
 protected:
  // NOLINTNEXTLINE(bugprone-random-generator-seed)
  std::mt19937 eng_{0};

  using Torus = typename Ctx::Torus;

  // ============================================================
  // test inputs
  // ============================================================

  Poly<Torus, Ctx::N> plaintext_;

  void SetUp() override {
    std::uniform_int_distribution<typename Torus::raw_value_type> torus_dist(
        Torus::raw_min(), Torus::raw_max());

    this->plaintext_ =
        Poly<Torus, Ctx::N>([&eng = this->eng_, &dist = torus_dist]() {
          return static_cast<Torus>(dist(eng));
        });
  }
};

TYPED_TEST_SUITE(GadgetReprTest, gadget_repr_test::TestContexts);

TYPED_TEST(GadgetReprTest, ReconstructsOriginalPolynomial) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  Poly<Torus, Ctx::N> poly = this->plaintext_;

  EXPECT_EQ(poly.size(), Ctx::N);

  GadgetRepr<Ctx> repr(poly);
  Poly<Torus, Ctx::N> reconstructed = repr.template reconstruct<Torus>();

  double error_norm = infinity_norm(poly - reconstructed);

  EXPECT_LE(error_norm, GadgetRepr<Ctx>::template threshold<Torus>);

  // clang-format off
  std::cout << "\n=== GadgetRepr Decomposition & Reconstruction Test ===\n";
  if (Ctx::verbose) {
  std::cout << "Original:      " << poly << "\n";
  std::cout << "Reconstructed: " << reconstructed << "\n";
  }

  std::cout << "Analysis:\n";
  std::cout << "  Infinity norm: " << error_norm << "\n";
  std::cout << "  Threshold: " << GadgetRepr<Ctx>::template threshold<Torus>
  << "\n"; std::cout <<
  "=====================================================\n\n";
  // clang-format on

  EXPECT_LT(error_norm, (GadgetRepr<Ctx>::template threshold<Torus>));
}