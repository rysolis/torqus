#include <gtest/gtest.h>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

TEST(PrimitiveConversionTest, UInt2ModInt) {
  using Zp = ModInt<12289>;
  UInt a(100);
  Zp x(static_cast<UInt::raw_value_type>(a));
  EXPECT_EQ(UInt(100), a);
  EXPECT_EQ(Zp(100), x);
}

TEST(PrimitiveConversionTest, UInt2ModInt_WrapsIntoModIntInterval) {
  using Zp = ModInt<99>;
  UInt a(100);
  Zp x(static_cast<UInt::raw_value_type>(a));
  EXPECT_EQ(UInt(100), a);
  EXPECT_EQ(Zp(1), x);
}

TEST(PrimitiveConversionTest, ModInt2UInt) {
  using Zp = ModInt<7>;
  Zp x(4);
  UInt a(static_cast<Zp::raw_value_type>(x));
  EXPECT_EQ(Zp(4), x);
  EXPECT_EQ(UInt(4), a);
}

TEST(PrimitiveConversionTest, ModTorus2ModInt) {
  using Zp = ModInt<12289>;
  using Torus = ModTorus<16>;
  Torus t(Torus::q - 1);
  Zp x(static_cast<Torus::raw_value_type>(t));
  EXPECT_EQ(Torus(Torus::q - 1), t);
  EXPECT_EQ(Zp(Torus::q - 1), x);
}

TEST(PrimitiveConversionTest, ModInt2ModTorus) {
  using Zp = ModInt<7>;
  using Torus = ModTorus<16>;
  Zp x(10);
  Torus t(static_cast<Zp::raw_value_type>(x));
  EXPECT_EQ(Zp(10), x);
  EXPECT_EQ(Torus(3), t);
}