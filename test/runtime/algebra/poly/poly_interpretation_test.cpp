#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

TEST(PolyInterpretationTest, CreatesIndependentPolyWithInterpretedValues) {
  Poly<ModTorus> src{ModTorus(1), ModTorus(2), ModTorus(3)};

  auto* raw_ptr = src.data();

  Poly<ModInt<100>> dst = interpret_as<ModInt<100>>(src);

  EXPECT_EQ(dst.size(), 3);

  EXPECT_EQ(dst[0],
            ModInt<100>(static_cast<ModTorus::raw_value_type>(ModTorus(1))));
  EXPECT_EQ(dst[1],
            ModInt<100>(static_cast<ModTorus::raw_value_type>(ModTorus(2))));
  EXPECT_EQ(dst[2],
            ModInt<100>(static_cast<ModTorus::raw_value_type>(ModTorus(3))));

  EXPECT_NE(dst.data(), raw_ptr);
}

TEST(PolyInterpretationTest, MoveBuffersPolyByInterpretation) {
  Poly<ModTorus> src{ModTorus(1), ModTorus(2), ModTorus(3)};

  auto* raw_ptr = src.data();

  Poly<ModInt<100>> dst = interpret_as<ModInt<100>>(std::move(src));

  EXPECT_EQ(dst.size(), 3);

  EXPECT_EQ(dst[0],
            ModInt<100>(static_cast<ModTorus::raw_value_type>(ModTorus(1))));
  EXPECT_EQ(dst[1],
            ModInt<100>(static_cast<ModTorus::raw_value_type>(ModTorus(2))));
  EXPECT_EQ(dst[2],
            ModInt<100>(static_cast<ModTorus::raw_value_type>(ModTorus(3))));

  EXPECT_EQ(dst.data(), raw_ptr);
}
