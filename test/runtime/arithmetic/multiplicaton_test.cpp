#include <gtest/gtest.h>

#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"

#include "arithmetic/negacyclic_convolution.hpp"

namespace multiplication_test {

struct Ctx1 {
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
};

using TestContests = ::testing::Types<Ctx1>;

}  // namespace multiplication_test

template <typename Ctx>
class ConvolutionTest : public ::testing::Test {};

TYPED_TEST_SUITE(ConvolutionTest, multiplication_test::TestContests);

TYPED_TEST(ConvolutionTest, IdentityCorrectly) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  Poly<UInt, Ctx::N> lhs{UInt(1), UInt(0), UInt(0), UInt(0)};
  Poly<Torus, Ctx::N> rhs{Torus(2), Torus(2), Torus(2), Torus(2)};

  Poly<Torus, Ctx::N> expected{Torus(2), Torus(2), Torus(2), Torus(2)};
  Poly<Torus, Ctx::N> result = negacyclic_convolution(lhs, rhs);

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}

TYPED_TEST(ConvolutionTest, ZeroPolynomial) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;
  Poly<UInt, Ctx::N> lhs{UInt(0), UInt(0), UInt(0), UInt(0)};
  Poly<Torus, Ctx::N> rhs{Torus(2), Torus(2), Torus(2), Torus(2)};

  Poly<Torus, Ctx::N> expected{Torus(0), Torus(0), Torus(0), Torus(0)};
  Poly<Torus, Ctx::N> result = negacyclic_convolution(lhs, rhs);

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}

TYPED_TEST(ConvolutionTest, WrapAround) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;
  Poly<UInt, Ctx::N> lhs{UInt(0), UInt(1), UInt(0), UInt(0)};
  Poly<Torus, Ctx::N> rhs{Torus(5), Torus(6), Torus(7), Torus(8)};

  Poly<Torus, Ctx::N> expected{Torus(Torus::raw_max() - 7), Torus(5), Torus(6),
                               Torus(7)};

  Poly<Torus, Ctx::N> result = negacyclic_convolution(lhs, rhs);

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}

TYPED_TEST(ConvolutionTest, DoubleApply) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;
  Poly<UInt, Ctx::N> lhs{UInt(0), UInt(1), UInt(0), UInt(0)};
  Poly<Torus, Ctx::N> rhs{Torus(5), Torus(6), Torus(7), Torus(8)};

  Poly<Torus, Ctx::N> expected{Torus(Torus::raw_max() - 6),
                               Torus(Torus::raw_max() - 7), Torus(5), Torus(6)};

  Poly<Torus, Ctx::N> result =
      negacyclic_convolution(lhs, negacyclic_convolution(lhs, rhs));

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}
