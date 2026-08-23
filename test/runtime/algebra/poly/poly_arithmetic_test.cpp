#include <gtest/gtest.h>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

#include "algebra/poly.hpp"

namespace poly_arithmetic_test {
using TestContexts = ::testing::Types<ModInt<7>, ModInt<32>, ModTorus<16>>;
}

template <typename T>
class PolyArithmeticTest : public ::testing::Test {};

TYPED_TEST_SUITE(PolyArithmeticTest, poly_arithmetic_test::TestContexts);

// N = 6 is not a multiple of the 4-lane NEON chunk width, so this exercises
// one full vectorized chunk (indices 0-3) plus a scalar tail (indices 4-5).
// b is constant at raw_max() (i.e. -1 mod P), so a[i] + b[i] wraps past the
// modulus for every i except i == 0, while a[i] - b[i] stays in range for
// every i -- values are picked so neither direction depends on which of
// the three moduli (7, 32, 65536) is in play.
TYPED_TEST(PolyArithmeticTest, OperatorPlusEquals_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Poly<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Poly<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
               T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  a += b;

  Poly<T, 6> expected{T(T::raw_max()), T(0), T(1), T(2), T(3), T(4)};
  EXPECT_EQ(expected, a);
}

TYPED_TEST(PolyArithmeticTest, OperatorMinusEquals_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Poly<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Poly<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
               T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  a -= b;

  Poly<T, 6> expected{T(1), T(2), T(3), T(4), T(5), T(6)};
  EXPECT_EQ(expected, a);
}

TYPED_TEST(PolyArithmeticTest, FreeOperatorPlus_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Poly<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Poly<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
               T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  Poly<T, 6> expected{T(T::raw_max()), T(0), T(1), T(2), T(3), T(4)};
  EXPECT_EQ(expected, a + b);
}

TYPED_TEST(PolyArithmeticTest, FreeOperatorMinus_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Poly<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Poly<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
               T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  Poly<T, 6> expected{T(1), T(2), T(3), T(4), T(5), T(6)};
  EXPECT_EQ(expected, a - b);
}

// ModInt<0> is the "unbounded" case (no modulus reduction at all), kept
// separate from the typed suite above since it has no raw_max() to anchor
// the value pattern used there.
TEST(PolyArithmeticModInt0Test, OperatorPlusEqualsAndMinusEquals_UnboundedNoReduction) {
  using T = ModInt<0>;

  Poly<T, 6> a{T(1), T(2), T(3), T(4), T(5), T(6)};
  Poly<T, 6> b{T(10), T(20), T(30), T(40), T(50), T(60)};

  Poly<T, 6> sum = a;
  sum += b;
  EXPECT_EQ((Poly<T, 6>{T(11), T(22), T(33), T(44), T(55), T(66)}), sum);

  Poly<T, 6> diff = b;
  diff -= a;
  EXPECT_EQ((Poly<T, 6>{T(9), T(18), T(27), T(36), T(45), T(54)}), diff);
}
