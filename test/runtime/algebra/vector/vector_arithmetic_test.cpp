#include <gtest/gtest.h>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

#include "algebra/vector.hpp"

namespace vector_arithmetic_test {
using TestContexts = ::testing::Types<ModInt<7>, ModInt<32>, ModTorus<16>>;
}

template <typename T>
class VectorArithmeticTest : public ::testing::Test {};

TYPED_TEST_SUITE(VectorArithmeticTest, vector_arithmetic_test::TestContexts);

// N = 6 is not a multiple of the 4-lane NEON chunk width, so this exercises
// one full vectorized chunk (indices 0-3) plus a scalar tail (indices 4-5).
// b is constant at raw_max() (i.e. -1 mod P), so a[i] + b[i] wraps past the
// modulus for every i except i == 0, while a[i] - b[i] stays in range for
// every i -- values are picked so neither direction depends on which of
// the three moduli (7, 32, 65536) is in play. Vector has no operator==, so
// elements are compared one at a time, as in vector_test.cpp.
TYPED_TEST(VectorArithmeticTest, OperatorPlusEquals_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Vector<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Vector<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
                 T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  a += b;

  EXPECT_EQ(T(T::raw_max()), a[0]);
  EXPECT_EQ(T(0), a[1]);
  EXPECT_EQ(T(1), a[2]);
  EXPECT_EQ(T(2), a[3]);
  EXPECT_EQ(T(3), a[4]);
  EXPECT_EQ(T(4), a[5]);
}

TYPED_TEST(VectorArithmeticTest, OperatorMinusEquals_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Vector<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Vector<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
                 T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  a -= b;

  EXPECT_EQ(T(1), a[0]);
  EXPECT_EQ(T(2), a[1]);
  EXPECT_EQ(T(3), a[2]);
  EXPECT_EQ(T(4), a[3]);
  EXPECT_EQ(T(5), a[4]);
  EXPECT_EQ(T(6), a[5]);
}

TYPED_TEST(VectorArithmeticTest, FreeOperatorPlus_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Vector<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Vector<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
                 T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  Vector<T, 6> sum = a + b;

  EXPECT_EQ(T(T::raw_max()), sum[0]);
  EXPECT_EQ(T(0), sum[1]);
  EXPECT_EQ(T(1), sum[2]);
  EXPECT_EQ(T(2), sum[3]);
  EXPECT_EQ(T(3), sum[4]);
  EXPECT_EQ(T(4), sum[5]);
}

TYPED_TEST(VectorArithmeticTest, FreeOperatorMinus_ReducesAcrossChunkAndTail) {
  using T = TypeParam;

  Vector<T, 6> a{T(0), T(1), T(2), T(3), T(4), T(5)};
  Vector<T, 6> b{T(T::raw_max()), T(T::raw_max()), T(T::raw_max()),
                 T(T::raw_max()), T(T::raw_max()), T(T::raw_max())};

  Vector<T, 6> diff = a - b;

  EXPECT_EQ(T(1), diff[0]);
  EXPECT_EQ(T(2), diff[1]);
  EXPECT_EQ(T(3), diff[2]);
  EXPECT_EQ(T(4), diff[3]);
  EXPECT_EQ(T(5), diff[4]);
  EXPECT_EQ(T(6), diff[5]);
}
