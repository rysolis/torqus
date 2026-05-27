#include <gtest/gtest.h>

#include <random>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"

struct Ctx1 {
  static constexpr uint32_t N = 4;
};

struct Ctx2 {
  static constexpr uint32_t N = 1024;
};

using TestContexts = ::testing::Types<Ctx1, Ctx2>;

template <typename Ctx>
class PolyConversionTest : public ::testing::Test {
 protected:
  std::mt19937 eng{std::random_device{}()};
  std::uniform_real_distribution<Torus::raw_value_type> uniform{0, 1.0};
};

TYPED_TEST_SUITE(PolyConversionTest, TestContexts);

TYPED_TEST(PolyConversionTest, ConvertPolyTorusLValue2PolyModTorus) {
  using Ctx = TypeParam;

  Poly<Torus> src(Ctx::N, [&eng = this->eng, &uniform = this->uniform]() {
    return static_cast<Torus>(uniform(eng));
  });
  Poly<ModTorus> dst = convert_to<ModTorus>(src);

  EXPECT_EQ(src.size(), Ctx::N);
  EXPECT_EQ(dst.size(), src.size());

  for (size_t i = 0; i < src.size(); ++i) {
    EXPECT_EQ(ModTorus(dst[i]),
              ModTorus(static_cast<ModTorus::raw_value_type>(
                  Torus::Q * static_cast<Torus::raw_value_type>(src[i]))));
  }
}

TYPED_TEST(PolyConversionTest, ConvertPolyTorusRValue2PolyModTorus) {
  using Ctx = TypeParam;

  Poly<Torus> original(Ctx::N, [&eng = this->eng, &uniform = this->uniform]() {
    return static_cast<Torus>(uniform(eng));
  });
  Poly<Torus> src = original;
  Poly<ModTorus> dst = convert_to<ModTorus>(std::move(src));

  EXPECT_EQ(original.size(), Ctx::N);
  EXPECT_EQ(dst.size(), original.size());

  for (size_t i = 0; i < dst.size(); ++i) {
    EXPECT_EQ(ModTorus(dst[i]),
              ModTorus(static_cast<ModTorus::raw_value_type>(
                  Torus::Q * static_cast<Torus::raw_value_type>(original[i]))));
  }
}
