#include <gtest/gtest.h>

#include "tfhe/structure/trgsw.hpp"

TEST(TrgswBasicTest, SizeConstructor_initializesBuffer) {
  TRGSW<ModTorus<16>> trgsw(4, 2);
  EXPECT_EQ(trgsw.level(), 4);
  for (size_t i = 0; i < trgsw.level(); ++i) {
    EXPECT_EQ(trgsw[i].a().size(), 4);
    EXPECT_EQ(trgsw[i].b().size(), 4);
  }
};