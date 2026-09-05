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
// Torus(1u, 4u) literals) -- a ciphertext encrypted from Dial(...).value()
// has to be usable by those gates unchanged. Resolution is 4, not 2: those
// gates sum two messages and compare against a single fixed decision
// boundary, which only works if each message stays under half the circle,
// so they reserve slots 2 and 3 as headroom and use only 0 and 1 -- Dial
// itself has no opinion about that; it just wraps a Torus value.
TYPED_TEST(DialTest, BoolMatchesExistingGateConvention) {
  using Torus = typename TypeParam::torus_type;
  using D = Dial<4, Torus>;

  EXPECT_EQ(D(0).value(), Torus(0u));
  EXPECT_EQ(D(1).value(), Torus(1u, 4u));

  // index accepts bool implicitly, so callers spell out intent directly
  // rather than remembering which index means what.
  EXPECT_EQ(D(false).value(), D(0).value());
  EXPECT_EQ(D(true).value(), D(1).value());
}

TYPED_TEST(DialTest, ConstructedFromIndexMatchesFormula) {
  using Torus = typename TypeParam::torus_type;

  {
    using D = Dial<4, Torus>;
    for (uint32_t i = 0; i < D::resolution; ++i) {
      EXPECT_EQ(D(i).value(), Torus(i, 4u));
    }
  }
  {
    using D = Dial<8, Torus>;
    for (uint32_t i = 0; i < D::resolution; ++i) {
      EXPECT_EQ(D(i).value(), Torus(i, 8u));
    }
  }
}

// Viewing the Torus sitting at a given index through a fresh Dial of the
// same shape reads back that same index -- the "window" round-trips.
TYPED_TEST(DialTest, IndexRoundTripsThroughConstruction) {
  using Torus = typename TypeParam::torus_type;
  using D = Dial<4, Torus>;

  for (uint32_t i = 0; i < D::resolution; ++i) {
    Torus raw = D(i).value();
    EXPECT_EQ(D(raw).index(), i);
  }
}

// index() must still resolve to the right slot under noise up to (but not
// past) margin(). i == 0 exercises the wraparound case: subtracting noise
// from slot 0 wraps to just below 1.
TYPED_TEST(DialTest, IndexToleratesNoiseWithinMargin) {
  using Torus = typename TypeParam::torus_type;
  using D = Dial<4, Torus>;

  // A fraction of D's own margin, built via Dial itself rather than
  // duplicating its numerator/denominator construction here.
  Torus tiny = Dial<64, Torus>(1).value();

  for (uint32_t i = 0; i < D::resolution; ++i) {
    Torus above = D(i).value();
    above += tiny;
    EXPECT_EQ(D(above).index(), i);

    Torus below = D(i).value();
    below -= tiny;
    EXPECT_EQ(D(below).index(), i);
  }
}

TYPED_TEST(DialTest, MarginIsHalfSlotWidth) {
  using Torus = typename TypeParam::torus_type;
  using D = Dial<4, Torus>;

  EXPECT_EQ(D::margin(), Torus(1u, 8u));
}
