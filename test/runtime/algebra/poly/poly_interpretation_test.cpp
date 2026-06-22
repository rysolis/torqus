#include <gtest/gtest.h>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

#include "algebra/poly.hpp"
#include "algebra/utility.hpp"

TEST(PolyInterpretationTest, CreatesIndependentPolyWithInterpretedValues) {
  using Torus = ModTorus<16>;
  Poly<Torus, 3> src{Torus(1), Torus(2), Torus(3)};

  auto* raw_ptr = src.data();

  Poly<ModInt<100>, 3> dst = interpret_as<ModInt<100>>(src);

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(1))), dst[0]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(2))), dst[1]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(3))), dst[2]);

  EXPECT_NE(raw_ptr, dst.data());
}

TEST(PolyInterpretationTest, MoveBuffersPolyByInterpretation) {
  using Torus = ModTorus<16>;
  Poly<Torus, 3> src{Torus(1), Torus(2), Torus(3)};

  auto* raw_ptr = src.data();

  Poly<ModInt<100>, 3> dst = interpret_as<ModInt<100>>(std::move(src));

  EXPECT_EQ(3, dst.size());

  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(1))), dst[0]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(2))), dst[1]);
  EXPECT_EQ(ModInt<100>(static_cast<Torus::raw_value_type>(Torus(3))), dst[2]);

  EXPECT_EQ(raw_ptr, dst.data());
}
