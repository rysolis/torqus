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

using TestContexts = ::testing::Types<Ctx1, Ctx2>;

}  // namespace trlwe_basic_test

template <typename Ctx>
class TrlweBasicTest : public ::testing::Test {};

TYPED_TEST_SUITE(TrlweBasicTest, trlwe_basic_test::TestContexts);

TYPED_TEST(TrlweBasicTest, SizeConstructor_initializesBuffer) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TRLWE<Torus, Ctx::N> trlwe;
  EXPECT_EQ(trlwe.a().size(), Ctx::N);
  EXPECT_EQ(trlwe.a(), (Poly<Torus, Ctx::N>()));
  EXPECT_EQ(trlwe.b().size(), Ctx::N);
  EXPECT_EQ(trlwe.b(), (Poly<Torus, Ctx::N>()));
}

TYPED_TEST(TrlweBasicTest, Generator_InitializedBuffer) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TRLWE<Torus, Ctx::N> trlwe([]() { return Torus(10u); });
  EXPECT_EQ(trlwe.a().size(), Ctx::N);
  EXPECT_EQ(trlwe.a(), (Poly<Torus, Ctx::N>([]() { return Torus(10u); })));
  EXPECT_EQ(trlwe.b().size(), Ctx::N);
  EXPECT_EQ(trlwe.b(), (Poly<Torus, Ctx::N>()));
}
