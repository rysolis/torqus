#include <gtest/gtest.h>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

TEST(PrimitiveConversionTest, UInt2ModInt) {
  using Zp = ModInt<12289>;
  UInt a(100);
  Zp x = static_cast<UInt::raw_value_type>(a);
  EXPECT_EQ(a, UInt(100));
  EXPECT_EQ(x, Zp(100));
}

TEST(PrimitiveConversionTest, UInt2ModInt_WrapsIntoModIntInterval) {
  using Zp = ModInt<99>;
  UInt a(100);
  Zp x = static_cast<UInt::raw_value_type>(a);
  EXPECT_EQ(a, UInt(100));
  EXPECT_EQ(x, Zp(1));
}

TEST(PrimitiveConversionTest, ModInt2UInt) {
  using Zp = ModInt<7>;
  Zp x(4);
  UInt a = static_cast<UInt>(static_cast<Zp::raw_value_type>(x));
  EXPECT_EQ(x, Zp(4));
  EXPECT_EQ(a, UInt(4));
}

TEST(PrimitiveConversionTest, ModTorus2ModInt) {
  using Zp = ModInt<12289>;
  ModTorus t(Torus::Q - 1);
  Zp x = static_cast<ModTorus::raw_value_type>(t);
  EXPECT_EQ(t, ModTorus(Torus::Q - 1));
  EXPECT_EQ(x, Zp(Torus::Q - 1));
}

TEST(PrimitiveConversionTest, ModInt2ModTorus) {
  using Zp = ModInt<7>;
  Zp x(10);
  ModTorus t = static_cast<ModTorus>(static_cast<Zp::raw_value_type>(x));
  EXPECT_EQ(x, Zp(10));
  EXPECT_EQ(t, ModTorus(3));
}