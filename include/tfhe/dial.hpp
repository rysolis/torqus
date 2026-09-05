// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_DIAL_HPP
#define TFHE_DIAL_HPP

#include <bit>
#include <cassert>
#include <concepts>
#include <cstdint>

#include "primitive/concept/torus.hpp"

// Torus types shaped like ModTorus<QBit, Word>: a (numerator, denominator)
// constructor, a raw_value_type, and a qbit -- everything Dial below needs
// to work in exact integer arithmetic, with no floating point involved.
template <typename Torus>
concept modtorus_concept = torus_concept<Torus> && requires(uint32_t i) {
  typename Torus::raw_value_type;
  { Torus::qbit } -> std::convertible_to<uint32_t>;
  Torus(i, i);
};

// Dial<Resolution, Torus> is the plaintext-space codec standing between a
// caller and the raw Torus values a scheme actually expects: it names each
// of Resolution logical values (Dial<4,...> for 4 values) so a caller
// never has to spell out the underlying fraction by hand.
//
// The Resolution values sit at i/Resolution for i in [0, Resolution),
// evenly spaced around the whole torus circle -- Dial has no opinion of
// its own about noise margins, decision boundaries, or how many of those
// slots a particular scheme actually uses; it is purely index <-> Torus
// value, nothing more.
//
// In particular, tfhe/gate/Hom* (HomAnd/HomOr/HomAndNot/HomXor) compute a
// 2-input gate by summing two encoded messages and comparing the sum
// against a single fixed decision boundary -- for that trick to work, the
// individual messages must stay confined to less than half the circle, or
// the sum of two of them could cross the boundary from the wrong side.
// Those gates get that headroom by asking Dial for twice as many slots as
// they have logical values and only ever using the first half of them
// (e.g. Dial<4, Torus> for a boolean compatible with them: at(0) == 0,
// at(1) == 1/4, matching mu = 1/4 in tfhe/gate/hom_and.hpp and friends;
// indices 2 and 3 are simply never used). That headroom requirement
// belongs to those gates, not to Dial -- a caller matching some other
// scheme's own convention picks whatever Resolution (and which of its
// slots) that scheme needs.
//
// Resolution has no default: how many slots a Dial has is a property of
// the scheme a caller is working with, never something to fall back on
// silently.
template <uint32_t Resolution, modtorus_concept Torus>
class Dial {
 public:
  static_assert(Resolution > 0, "Dial needs at least one slot");
  // Every plaintext modulus this library actually builds is a power of
  // two (see tfhe/math/modswitch.hpp's own equivalent restriction);
  // requiring it here up front keeps at()/decode()/margin() exact integer
  // arithmetic throughout, with no double anywhere in this class.
  static_assert((Resolution & (Resolution - 1)) == 0,
                "Dial needs Resolution to be a power of two");

  using Word = typename Torus::raw_value_type;
  static constexpr uint32_t qbit = Torus::qbit;
  static constexpr uint32_t resolution = Resolution;

  // log2(Resolution) -- how many of qbit's top bits identify a slot.
  static constexpr uint32_t k =
      static_cast<uint32_t>(std::bit_width(Resolution - 1));
  static_assert(qbit > k,
                "Resolution has more slots than this Torus's qbit can "
                "distinguish");

  // The Torus value naming slot `index` (index must be < Resolution).
  // `index` accepts bool implicitly, so a caller building a Dial<2, ...>
  // can write at(true)/at(false) directly instead of at(1)/at(0).
  static constexpr Torus at(uint32_t index) {
    assert(index < Resolution);
    return Torus(index, Resolution);
  }

  // The slot nearest `t`, tolerating up to half a slot's worth of noise --
  // see margin(). Exact integer arithmetic throughout: the same narrowing
  // "round to nearest via a half-ULP offset, then shift" trick
  // tfhe/math/modswitch.hpp's own mod_switch already uses to move a
  // Torus's raw value down to a smaller power-of-two modulus, applied
  // here to shrink it to Resolution slots instead. Going through double
  // here instead would silently lose bits once qbit exceeds double's
  // 52-bit mantissa (e.g. ModTorus<64, uint64_t>), which is exactly the
  // kind of noisy-decrypt-off-by-one this codec exists to not introduce.
  static constexpr uint32_t decode(const Torus& t) {
    constexpr uint32_t drop = qbit - k;
    constexpr Word half = Word{1} << (drop - 1);
    constexpr Word slot_mask = static_cast<Word>((Word{1} << k) - 1);

    Word raw = static_cast<Word>(t.value());
    // raw + half can wrap Word's width when qbit is Word's full width
    // (ModTorus's own "mod == 0" sentinel) -- harmless, since the extra
    // bit carried past the top is an exact multiple of 2^k (== Resolution)
    // that slot_mask discards next, same as mod_switch's own reasoning.
    Word idx = static_cast<Word>((raw + half) >> drop) & slot_mask;
    return static_cast<uint32_t>(idx);
  }

  // Half a slot's width: the largest noise magnitude decode() still
  // resolves to the intended slot.
  static constexpr Torus margin() { return Torus(1u, 2u * Resolution); }
};

#endif  // TFHE_DIAL_HPP
