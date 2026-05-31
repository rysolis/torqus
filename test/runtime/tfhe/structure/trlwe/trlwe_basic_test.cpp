#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"
#include "tfhe/structure/trlwe.hpp"

namespace trlwe_basic_test {
struct Ctx1 {
  using Torus = ModTorus<16>;
  static constexpr uint32_t N = 4;
};

struct Ctx2 {
  using Torus = ModTorus<8>;
  static constexpr uint32_t N = 32;
};

}  // namespace trlwe_basic_test

template <typename Ctx>
class TrlweBasicTest : public ::testing::Test {};

using TestContexts =
    ::testing::Types<trlwe_basic_test::Ctx1, trlwe_basic_test::Ctx2>;

TYPED_TEST_SUITE(TrlweBasicTest, TestContexts);

TYPED_TEST(TrlweBasicTest, SizeConstructor_initializesBuffer) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TRLWE<Torus> trlwe(Ctx::N);
  EXPECT_EQ(trlwe.a().size(), Ctx::N);
  EXPECT_EQ(trlwe.a(), Poly<Torus>(Ctx::N));
  EXPECT_EQ(trlwe.b().size(), Ctx::N);
  EXPECT_EQ(trlwe.b(), Poly<Torus>(Ctx::N));
}

TYPED_TEST(TrlweBasicTest, Generator_InitializedBuffer) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TRLWE<Torus> trlwe(Ctx::N, []() { return Torus(10u); });
  EXPECT_EQ(trlwe.a().size(), Ctx::N);
  EXPECT_EQ(trlwe.a(), Poly<Torus>(Ctx::N, []() { return Torus(10u); }));
  EXPECT_EQ(trlwe.b().size(), Ctx::N);
  EXPECT_EQ(trlwe.b(), Poly<Torus>(Ctx::N));
}
