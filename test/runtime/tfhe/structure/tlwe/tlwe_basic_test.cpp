#include <gtest/gtest.h>

#include "primitive/torus.hpp"

#include "algebra/poly.hpp"

#include "tfhe/structure/tlwe.hpp"

namespace trlwe_basic_test {
struct Ctx1 {
  using Torus = ModTorus<16>;
  static constexpr uint32_t n = 10;
};

using TestContexts = ::testing::Types<Ctx1>;
}  // namespace trlwe_basic_test

template <typename Ctx>
class TlweBasicTest : public ::testing::Test {};

TYPED_TEST_SUITE(TlweBasicTest, trlwe_basic_test::TestContexts);

TYPED_TEST(TlweBasicTest, Constructor) {
  using Ctx = TypeParam;
  using Torus = typename Ctx::Torus;

  TLWE<Torus, Ctx::n> tlwe;

  for (size_t i = 0; i < Ctx::n; ++i) {
    EXPECT_EQ(Torus(0), tlwe.a()[i]);
  }
  EXPECT_EQ(Torus(0), tlwe.b());
}
