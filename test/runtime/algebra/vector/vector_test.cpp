#include <gtest/gtest.h>

#include "primitive/modint.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

namespace vector_test {
using TestContexts = ::testing::Types<UInt>;
}

template <typename T>
class VectorBasicTest : public ::testing::Test {};

TYPED_TEST_SUITE(VectorBasicTest, vector_test::TestContexts);

TYPED_TEST(VectorBasicTest, SizeConstructor_InitializesBuffer) {
  using T = TypeParam;

  Vector<T, 4> v;
  EXPECT_EQ(4, v.size());

  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(T(0), v[i]);
  }
}

TYPED_TEST(VectorBasicTest, ConstructFromPolynomial) {
  using T = TypeParam;

  Poly<T, 4> p{T(1), T(2), T(3), T(4)};
  Vector<T, 4> v(p.begin(), p.end());

  EXPECT_EQ(4, v.size());

  for (size_t i = 0; i < v.size(); ++i) {
    EXPECT_EQ(static_cast<T>(p[i]), v[i]);
  }
}

TYPED_TEST(VectorBasicTest, SubscriptOperator) {
  using T = TypeParam;
  Vector<T, 4> p{T(0), T(1), T(2), T(3)};

  T add = p[0] + p[1];

  EXPECT_EQ(T(0), p[0]);
  EXPECT_EQ(T(1), p[1]);
  EXPECT_EQ(T(1), add);

  if constexpr (std::same_as<T, UInt>) {
    EXPECT_THROW({ p[2] - p[3]; }, std::underflow_error);
  } else {
    T sub = p[2] - p[3];
    EXPECT_EQ(T(2), p[2]);
    if constexpr (std::same_as<T, ModInt<7>>) {
      EXPECT_EQ(T(6), sub);
    } else {
      EXPECT_EQ(T(static_cast<T::raw_value_type>(-1)), sub);
    }
  }
  EXPECT_EQ(T(3), p[3]);

  p[2] = static_cast<T>(p[3]);
  EXPECT_EQ(T(3), p[2]);
  EXPECT_EQ(T(3), p[3]);
}
