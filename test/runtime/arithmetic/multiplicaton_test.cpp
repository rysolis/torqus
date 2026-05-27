#include <gtest/gtest.h>

#include "arithmetic/multiplication.hpp"

TEST(MultiplicationTest, IdentityCorrectly) {
  Poly<UInt> lhs{UInt(1), UInt(0), UInt(0), UInt(0)};
  Poly<ModTorus> rhs{ModTorus(2), ModTorus(2), ModTorus(2), ModTorus(2)};

  Poly<ModTorus> expected{ModTorus(2), ModTorus(2), ModTorus(2), ModTorus(2)};
  Poly<ModTorus> result = lhs * rhs;

  EXPECT_EQ(result.size(), expected.size());
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_EQ(ModTorus(result[i]), ModTorus(expected[i]));
  }
}

TEST(MultiplicationTest, ZeroPolynomial) {
  Poly<UInt> lhs{UInt(0), UInt(0), UInt(0), UInt(0)};
  Poly<ModTorus> rhs{ModTorus(2), ModTorus(2), ModTorus(2), ModTorus(2)};

  Poly<ModTorus> expected{ModTorus(0), ModTorus(0), ModTorus(0), ModTorus(0)};
  Poly<ModTorus> result = lhs * rhs;

  EXPECT_EQ(result.size(), expected.size());
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_EQ(ModTorus(result[i]), ModTorus(expected[i]));
  }
}

TEST(MultiplicationTest, DifferentSizes) {
  Poly<UInt> lhs{UInt(1), UInt(2), UInt(3), UInt(4), UInt(5)};
  Poly<ModTorus> rhs{ModTorus(5), ModTorus(6), ModTorus(7), ModTorus(8)};

  Poly<ModTorus> expected{ModTorus(5), ModTorus(16), ModTorus(34),
                          ModTorus(60)};

  EXPECT_DEBUG_DEATH({ Poly<ModTorus> result = lhs * rhs; }, ".*");
}

TEST(MultiplicationTest, WrapAround) {
  Poly<UInt> lhs{UInt(0), UInt(1), UInt(0), UInt(0)};
  Poly<ModTorus> rhs{ModTorus(5), ModTorus(6), ModTorus(7), ModTorus(8)};

  Poly<ModTorus> expected{ModTorus(Torus::Q - 8), ModTorus(5), ModTorus(6),
                          ModTorus(7)};

  Poly<ModTorus> result = lhs * rhs;

  EXPECT_EQ(result.size(), expected.size());
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_EQ(ModTorus(result[i]), ModTorus(expected[i]));
  }
}

TEST(MultiplicationTest, VerifyConsistency) {
  Poly<UInt> lhs{UInt(1), UInt(2), UInt(3), UInt(4)};
  Poly<ModTorus> rhs{ModTorus(5), ModTorus(6), ModTorus(7), ModTorus(8)};

  Poly<ModTorus> result = lhs * rhs;

  Poly<Torus> reference = lhs * convert_to<Torus>(rhs);

  EXPECT_EQ(result.size(), reference.size());
  for (size_t i = 0; i < result.size(); ++i) {
    EXPECT_EQ(static_cast<Torus>(ModTorus(result[i])), Torus(reference[i]));
  }
}