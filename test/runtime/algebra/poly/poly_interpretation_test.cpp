#include <gtest/gtest.h>

#include "algebra/poly.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

TEST(PolyInterpretationTest, CreatesIndependentPolyWithInterpretedValues) {
  using Torus = ModTorus<16>;
  Poly<Torus> src{Torus(1), Torus(2), Torus(3)};

  auto* raw_ptr = src.data();

  Poly<ModInt<100>> dst = interpret_as<ModInt<100>>(src);

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(1))), dst[0]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(2))), dst[1]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(3))), dst[2]);

  EXPECT_NE(dst.data(), raw_ptr);
}

TEST(PolyInterpretationTest, MoveBuffersPolyByInterpretation) {
  using Torus = ModTorus<16>;
  Poly<Torus> src{Torus(1), Torus(2), Torus(3)};

  auto* raw_ptr = src.data();

  Poly<ModInt<100>> dst = interpret_as<ModInt<100>>(std::move(src));

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(1))), dst[0]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(2))), dst[1]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(3))), dst[2]);

  EXPECT_EQ(dst.data(), raw_ptr);
}
