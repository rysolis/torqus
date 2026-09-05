#include <gtest/gtest.h>

#include <cstdint>

#include "primitive/torus.hpp"

#include "tfhe/dial.hpp"

namespace dial_test {
struct Ctx1 {
  using torus_type = ModTorus<8>;
};
struct Ctx2 {
  using torus_type = ModTorus<16>;
};
struct Ctx3 {
  using torus_type = ModTorus<32>;
};
struct Ctx4 {
  using torus_type = ModTorus<64, uint64_t>;
};
using TestContexts = ::testing::Types<Ctx1, Ctx2, Ctx3, Ctx4>;
}  // namespace dial_test

template <typename Ctx>
class DialTest : public ::testing::Test {};

TYPED_TEST_SUITE(DialTest, dial_test::TestContexts);

// Dial<2, Torus> must reproduce the exact {0, 1/4} boolean encoding every
// tfhe/gate/Hom* already relies on (see hom_and_test.cpp's Torus(0u) /
// Torus(1u, 4u) literals) -- a ciphertext encrypted from Bit::at(...) has
// to be usable by those gates unchanged.
TYPED_TEST(DialTest, BoolMatchesExistingGateConvention) {
  using Torus = typename TypeParam::torus_type;
  using Bit = Dial<2, Torus>;

  EXPECT_EQ(Bit::at(0), Torus(0u));
  EXPECT_EQ(Bit::at(1), Torus(1u, 4u));

  // bool converts implicitly to the slot index, so callers spell out
  // intent directly rather than remembering which index means what.
  EXPECT_EQ(Bit::at(false), Bit::at(0));
  EXPECT_EQ(Bit::at(true), Bit::at(1));
}

TYPED_TEST(DialTest, AtMatchesFormulaForVariousResolutions) {
  using Torus = typename TypeParam::torus_type;

  {
    using D = Dial<4, Torus>;
    for (uint32_t i = 0; i < D::resolution; ++i) {
      EXPECT_EQ(D::at(i), Torus(i, 8u));
    }
  }
  {
    using D = Dial<8, Torus>;
    for (uint32_t i = 0; i < D::resolution; ++i) {
      EXPECT_EQ(D::at(i), Torus(i, 16u));
    }
  }
}

TYPED_TEST(DialTest, DecodeRoundTripsThroughAt) {
  using Torus = typename TypeParam::torus_type;
  using Bit = Dial<4, Torus>;

  for (uint32_t i = 0; i < Bit::resolution; ++i) {
    EXPECT_EQ(Bit::decode(Bit::at(i)), i);
  }
}

// decode() must still resolve to the right slot under noise up to (but not
// past) margin() -- the same tolerance hom_and_test.cpp's hand-written
// decode_margin already checks correctness against. i == 0 exercises the
// wraparound case: subtracting noise from slot 0 wraps to just below 1.
TYPED_TEST(DialTest, DecodeToleratesNoiseWithinMargin) {
  using Torus = typename TypeParam::torus_type;
  using Bit = Dial<4, Torus>;

  // A quarter of Bit's own margin, built via Dial itself rather than
  // duplicating its numerator/denominator construction here.
  Torus tiny = Dial<64, Torus>::at(1);

  for (uint32_t i = 0; i < Bit::resolution; ++i) {
    Torus above = Bit::at(i);
    above += tiny;
    EXPECT_EQ(Bit::decode(above), i);

    Torus below = Bit::at(i);
    below -= tiny;
    EXPECT_EQ(Bit::decode(below), i);
  }
}

TYPED_TEST(DialTest, MarginIsHalfSlotWidth) {
  using Torus = typename TypeParam::torus_type;
  using Bit = Dial<4, Torus>;

  EXPECT_EQ(Bit::margin(), Torus(1u, 16u));
}
