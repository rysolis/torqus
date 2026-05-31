#include <gtest/gtest.h>

#include "arithmetic/multiplication.hpp"

TEST(MultiplicationTest, IdentityCorrectly) {
  using Torus = ModTorus<16>;
  Poly<UInt> lhs{UInt(1), UInt(0), UInt(0), UInt(0)};
  Poly<Torus> rhs{Torus(2), Torus(2), Torus(2), Torus(2)};

  Poly<Torus> expected{Torus(2), Torus(2), Torus(2), Torus(2)};
  Poly<Torus> result = lhs * rhs;

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}

TEST(MultiplicationTest, ZeroPolynomial) {
  using Torus = ModTorus<16>;
  Poly<UInt> lhs{UInt(0), UInt(0), UInt(0), UInt(0)};
  Poly<Torus> rhs{Torus(2), Torus(2), Torus(2), Torus(2)};

  Poly<Torus> expected{Torus(0), Torus(0), Torus(0), Torus(0)};
  Poly<Torus> result = lhs * rhs;

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}

TEST(MultiplicationTest, DifferentSizes) {
  using Torus = ModTorus<16>;
  Poly<UInt> lhs{UInt(1), UInt(2), UInt(3), UInt(4), UInt(5)};
  Poly<Torus> rhs{Torus(5), Torus(6), Torus(7), Torus(8)};

  Poly<Torus> expected{Torus(5), Torus(16), Torus(34), Torus(60)};

  EXPECT_DEBUG_DEATH({ Poly<Torus> result = lhs * rhs; }, ".*");
}

TEST(MultiplicationTest, WrapAround) {
  using Torus = ModTorus<16>;
  Poly<UInt> lhs{UInt(0), UInt(1), UInt(0), UInt(0)};
  Poly<Torus> rhs{Torus(5), Torus(6), Torus(7), Torus(8)};

  Poly<Torus> expected{Torus(Torus::q - 8), Torus(5), Torus(6), Torus(7)};

  Poly<Torus> result = lhs * rhs;

  EXPECT_EQ(expected.size(), result.size());
  EXPECT_EQ(expected, result);
}
