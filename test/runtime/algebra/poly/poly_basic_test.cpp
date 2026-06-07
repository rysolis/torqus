#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

namespace poly_basic_test {
using TestContexts = ::testing::Types<UInt, ModTorus<16>, ModInt<7>>;
}

template <typename T>
class PolyBasicTest : public ::testing::Test {};

TYPED_TEST_SUITE(PolyBasicTest, poly_basic_test::TestContexts);

TYPED_TEST(PolyBasicTest, SizeConstructor_InitializesBuffer) {
  using T = TypeParam;

  Poly<T, 4> p;
  EXPECT_EQ(4, p.size());

  for (size_t i = 0; i < p.size(); ++i) {
    EXPECT_EQ(T(0), p[i]);
  }
}

TYPED_TEST(PolyBasicTest, Generator_InitializedBuffer) {
  using T = TypeParam;

  Poly<T, 4> p([]() { return static_cast<T>(1); });
  EXPECT_EQ(4, p.size());

  for (size_t i = 0; i < p.size(); ++i) {
    EXPECT_EQ(T(1), p[i]);
  }
}

TYPED_TEST(PolyBasicTest, RawPointerConstructor_InitializesBuffer) {
  using T = TypeParam;

  typename T::raw_value_type ptr[] = {1, 2, 3, 4};
  Poly<T, 4> p(std::begin(ptr), std::end(ptr));
  EXPECT_EQ(4, p.size());

  for (size_t i = 0; i < p.size(); ++i) {
    EXPECT_EQ(T(ptr[i]), p[i]);
  }
}

TYPED_TEST(PolyBasicTest, InitializerListConstructor_InitializesBuffer) {
  using T = TypeParam;

  Poly<T, 4> p{T(0), T(1), T(2), T(3)};
  EXPECT_EQ(4, p.size());

  for (typename T::raw_value_type i = 0; i < p.size(); ++i) {
    EXPECT_EQ(T(i), p[i]);
  }
}

TYPED_TEST(PolyBasicTest, CopyConstructor_CopiesBuffer) {
  using T = TypeParam;

  Poly<T, 3> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T, 3> dst = src;

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(T(1), dst[0]);
  EXPECT_EQ(T(2), dst[1]);
  EXPECT_EQ(T(3), dst[2]);

  EXPECT_NE(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, CopyAssignOperator_CopiesBuffer) {
  using T = TypeParam;

  Poly<T, 3> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T, 3> dst;
  dst = src;

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(T(1), dst[0]);
  EXPECT_EQ(T(2), dst[1]);
  EXPECT_EQ(T(3), dst[2]);

  EXPECT_NE(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, MoveConstructor_MovesBuffer) {
  using T = TypeParam;

  Poly<T, 3> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T, 3> dst = std::move(src);

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(T(1), dst[0]);
  EXPECT_EQ(T(2), dst[1]);
  EXPECT_EQ(T(3), dst[2]);

  EXPECT_EQ(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, MoveAssignOperator_MovesBuffer) {
  using T = TypeParam;

  Poly<T, 3> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T, 3> dst;
  dst = std::move(src);

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(T(1), dst[0]);
  EXPECT_EQ(T(2), dst[1]);
  EXPECT_EQ(T(3), dst[2]);

  EXPECT_EQ(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, SubscriptOperator) {
  using T = TypeParam;

  Poly<T, 4> p{T(0), T(1), T(2), T(3)};

  T add = static_cast<T>(p[0]) + static_cast<T>(p[1]);
  EXPECT_EQ(T(0), p[0]);
  EXPECT_EQ(T(1), p[1]);
  EXPECT_EQ(T(1), add);

  if constexpr (std::same_as<T, UInt>) {
    EXPECT_THROW({ p[2] - p[3]; }, std::underflow_error);
  } else {
    T sub = static_cast<T>(p[2]) - static_cast<T>(p[3]);
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
