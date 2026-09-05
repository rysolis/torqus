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

// Dial<4, Torus> reproduces the exact {0, 1/4} boolean encoding
// tfhe/gate/Hom* already relies on (see hom_and_test.cpp's Torus(0u) /
// Torus(1u, 4u) literals) -- a ciphertext encrypted from at(...) has to be
// usable by those gates unchanged. Resolution is 4, not 2: those gates
// sum two messages and compare against a single fixed decision boundary,
// which only works if each message stays under half the circle, so they
// reserve slots 2 and 3 as headroom and use only 0 and 1 -- Dial itself
// has no opinion about that; it is purely index <-> Torus value.
TYPED_TEST(DialTest, BoolMatchesExistingGateConvention) {
  using Torus = typename TypeParam::torus_type;
  using Bit = Dial<4, Torus>;

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
      EXPECT_EQ(D::at(i), Torus(i, 4u));
    }
  }
  {
    using D = Dial<8, Torus>;
    for (uint32_t i = 0; i < D::resolution; ++i) {
      EXPECT_EQ(D::at(i), Torus(i, 8u));
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
// past) margin(). i == 0 exercises the wraparound case: subtracting noise
// from slot 0 wraps to just below 1.
TYPED_TEST(DialTest, DecodeToleratesNoiseWithinMargin) {
  using Torus = typename TypeParam::torus_type;
  using Bit = Dial<4, Torus>;

  // A fraction of Bit's own margin, built via Dial itself rather than
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

  EXPECT_EQ(Bit::margin(), Torus(1u, 8u));
}
