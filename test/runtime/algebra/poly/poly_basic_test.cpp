#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

template <typename T>
class PolyBasicTest : public ::testing::Test {};

using PolyTestTypes = ::testing::Types<UInt, ModTorus<16>>;

TYPED_TEST_SUITE(PolyBasicTest, PolyTestTypes);

TYPED_TEST(PolyBasicTest, SizeConstructor_InitializesBuffer) {
  using T = TypeParam;

  Poly<T> p(4);
  EXPECT_EQ(p.size(), 4);

  for (size_t i = 0; i < p.size(); ++i) {
    EXPECT_EQ(p[i], T(0));
  }
}

TYPED_TEST(PolyBasicTest, Generator_InitializedBuffer) {
  using T = TypeParam;

  Poly<T> p(4, []() { return static_cast<T>(1); });
  EXPECT_EQ(p.size(), 4);

  for (size_t i = 0; i < p.size(); ++i) {
    EXPECT_EQ(p[i], T(1));
  }
}

TYPED_TEST(PolyBasicTest, RawPointerConstructor_InitializesBuffer) {
  using T = TypeParam;

  UInt::raw_value_type ptr[] = {1, 2, 3, 4};
  Poly<T> p(ptr, 4);
  EXPECT_EQ(p.size(), 4);

  for (size_t i = 0; i < p.size(); ++i) {
    EXPECT_EQ(p[i], T(ptr[i]));
  }
}

TYPED_TEST(PolyBasicTest, UniquePtrConstructor_InitializesBuffer) {
  using T = TypeParam;

  std::unique_ptr<typename T::raw_value_type[]> ptr =
      std::make_unique<typename T::raw_value_type[]>(4);
  ptr[0] = 0;
  ptr[1] = 1;
  ptr[2] = 2;
  ptr[3] = 3;

  auto* raw_ptr = ptr.get();

  Poly<T> dst(std::move(ptr), 4);
  EXPECT_EQ(dst.size(), 4);

  for (typename T::raw_value_type i = 0; i < dst.size(); ++i) {
    EXPECT_EQ(dst[i], T(i));
  }

  EXPECT_EQ(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, InitializerListConstructor_InitializesBuffer) {
  using T = TypeParam;

  Poly<T> p{T(0), T(1), T(2), T(3)};
  EXPECT_EQ(p.size(), 4);

  for (typename T::raw_value_type i = 0; i < p.size(); ++i) {
    EXPECT_EQ(p[i], T(i));
  }
}

TYPED_TEST(PolyBasicTest, CopyConstructor_CopiesBuffer) {
  using T = TypeParam;

  Poly<T> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T> dst = src;

  EXPECT_EQ(dst.size(), 3);

  EXPECT_EQ(dst[0], T(1));
  EXPECT_EQ(dst[1], T(2));
  EXPECT_EQ(dst[2], T(3));

  EXPECT_NE(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, CopyAssignOperator_CopiesBuffer) {
  using T = TypeParam;

  Poly<T> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T> dst(3);
  dst = src;

  EXPECT_EQ(dst.size(), 3);

  EXPECT_EQ(dst[0], T(1));
  EXPECT_EQ(dst[1], T(2));
  EXPECT_EQ(dst[2], T(3));

  EXPECT_NE(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, MoveConstructor_MovesBuffer) {
  using T = TypeParam;

  Poly<T> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T> dst = std::move(src);

  EXPECT_EQ(dst.size(), 3);

  EXPECT_EQ(dst[0], T(1));
  EXPECT_EQ(dst[1], T(2));
  EXPECT_EQ(dst[2], T(3));

  EXPECT_EQ(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, MoveAssignOperator_MovesBuffer) {
  using T = TypeParam;

  Poly<T> src{T(1), T(2), T(3)};

  auto* raw_ptr = src.data();

  Poly<T> dst(3);
  dst = std::move(src);

  EXPECT_EQ(dst.size(), 3);

  EXPECT_EQ(dst[0], T(1));
  EXPECT_EQ(dst[1], T(2));
  EXPECT_EQ(dst[2], T(3));

  EXPECT_EQ(dst.data(), raw_ptr);
}

TYPED_TEST(PolyBasicTest, SubscriptOperator) {
  using T = TypeParam;

  Poly<T> p{T(0), T(1), T(2), T(3)};

  T add = static_cast<T>(p[0]) + static_cast<T>(p[1]);
  EXPECT_EQ(p[0], T(0));
  EXPECT_EQ(p[1], T(1));
  EXPECT_EQ(add, T(1));

  if constexpr (std::same_as<T, UInt>) {
    EXPECT_THROW({ p[2] - p[3]; }, std::underflow_error);
  } else {
    T sub = static_cast<T>(p[2]) - static_cast<T>(p[3]);
    EXPECT_EQ(p[2], T(2));
    EXPECT_EQ(sub, T(static_cast<T::raw_value_type>(-1)));
  }
  EXPECT_EQ(p[3], T(3));

  p[2] = static_cast<T>(p[3]);
  EXPECT_EQ(p[2], T(3));
  EXPECT_EQ(p[3], T(3));
}
