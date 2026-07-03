#include <gtest/gtest.h>

#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

namespace vector_test {
using TestContexts = ::testing::Types<UInt>;
}

template <typename T>
class VectorTest : public ::testing::Test {};

TYPED_TEST_SUITE(VectorTest, vector_test::TestContexts);

TYPED_TEST(VectorTest, SizeConstructor_InitializesBuffer) {
  using T = TypeParam;

  Vector<T, 4> v;
  EXPECT_EQ(4, v.size());

  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(T(0), v[i]);
  }
}

TYPED_TEST(VectorTest, ConstructFromPolynomial) {
  using T = TypeParam;

  Poly<T, 4> p{T(1), T(2), T(3), T(4)};
  Vector<T, 4> v(p.begin(), p.end());

  EXPECT_EQ(4, v.size());

  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(static_cast<T>(p[i]), v[i]);
  }
}