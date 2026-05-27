#include <gtest/gtest.h>

#include "primitive/torus.hpp"

// Convert Torus to ModTorus and back, and check if the value is
// preserved
TEST(ModTorusConvertTest, ConvertTorusLValue2ModTorus) {
  Torus dt(0.5);
  ModTorus mt = static_cast<ModTorus>(dt);

  EXPECT_EQ(static_cast<ModTorus::raw_value_type>(mt), Torus::Q / 2);
}

TEST(ModTorusConvertTest, ConvertTorusRValue2ModTorus) {
  ModTorus mt = static_cast<ModTorus>(Torus(0.5));

  EXPECT_EQ(static_cast<ModTorus::raw_value_type>(mt), Torus::Q / 2);
}

// Convert ModTorus to Torus and back, and check if the value is
// preserved
TEST(TorusConvertTest, ConvertModTorusLValue2Torus) {
  ModTorus mt(Torus::Q / 2);
  Torus dt = static_cast<Torus>(mt);

  EXPECT_NEAR(static_cast<Torus::raw_value_type>(dt), 0.5, 1e-9);
}

TEST(TorusConvertTest, ConvertModTorusRValue2Torus) {
  Torus dt = static_cast<Torus>(ModTorus(Torus::Q / 2));

  EXPECT_NEAR(static_cast<Torus::raw_value_type>(dt), 0.5, 1e-9);
}