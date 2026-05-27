#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"
#include "tfhe/structure/trlwe.hpp"

TEST(TrlweBasicTest, SizeConstructor_initializesBuffer) {
  TRLWE trlwe(4);
  EXPECT_EQ(trlwe.a().size(), 4);
  EXPECT_EQ(trlwe.a(), Poly<Torus>(4));
  EXPECT_EQ(trlwe.b().size(), 4);
  EXPECT_EQ(trlwe.b(), Poly<Torus>(4));
}

TEST(TrlweBasicTest, Generator_InitializedBuffer) {
  TRLWE trlwe(4, []() { return 0.1; });
  EXPECT_EQ(trlwe.a().size(), 4);
  EXPECT_EQ(trlwe.a(),
            Poly<Torus>(4, []() { return static_cast<Torus>(0.1); }));
  EXPECT_EQ(trlwe.b().size(), 4);
  EXPECT_EQ(trlwe.b(), Poly<Torus>(4));
}
