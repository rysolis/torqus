#include <gtest/gtest.h>

#include "tfhe/structure/trgsw.hpp"

TEST(TrgswBasicTest, SizeConstructor_initializesBuffer) {
  TRGSW<ModTorus<16>, 4, 2> trgsw;
  EXPECT_EQ(trgsw.level(), 4);
  for (size_t i = 0; i < trgsw.level(); ++i) {
    EXPECT_EQ(4, trgsw[i].a().size());
    EXPECT_EQ(4, trgsw[i].b().size());
  }
};