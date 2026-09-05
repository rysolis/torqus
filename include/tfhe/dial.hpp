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
// caller and the raw Torus values tfhe/gate/Hom* actually expect: it names
// each of Resolution logical values (Dial<2,...> for a bool) so a caller
// never has to spell out the underlying fraction by hand.
//
// The Resolution values sit at i/(2*Resolution) for i in [0, Resolution),
// i.e. spaced 1/(2*Resolution) apart starting at 0 and covering only the
// lower half [0, 1/2) of the torus circle -- the upper half is left as
// bootstrapping headroom. This isn't a fresh convention invented here: for
// Resolution=2 it's exactly the existing {0, 1/4} used throughout
// tfhe/gate/Hom* (mu = 1/(2*Resolution) = 1/4, added back post-bootstrap
// by tfhe/operation/bootstrap/gate_bootstrap.hpp's `mu.value() / 2`
// offset), so Dial<2, Torus>::at(i) is drop-in compatible with every
// existing gate -- no gate needed to change to adopt it. Resolution > 2
// generalizes that same spacing; nothing in this codebase exercises that
// yet, but the formula reduces to the Resolution=2 case exactly, so it
// costs nothing to keep general.
//
// Resolution has no default: how many slots a Dial has is a property of
// the scheme a caller is working with (matching a specific gate's own
// convention, as above), never something to fall back on silently.
template <uint32_t Resolution, modtorus_concept Torus>
class Dial {
 public:
  static_assert(Resolution > 0, "Dial needs at least one slot");
  // Every plaintext modulus this library actually builds is a power of
  // two (see tfhe/math/modswitch.hpp's own equivalent restriction);
  // requiring it here up front keeps at()/decode()/margin() exact integer
  // arithmetic throughout, with no double anywhere in this class.
  static_assert(((2u * Resolution) & (2u * Resolution - 1)) == 0,
                "Dial needs 2*Resolution to be a power of two");

  using Word = typename Torus::raw_value_type;
  static constexpr uint32_t qbit = Torus::qbit;
  static constexpr uint32_t resolution = Resolution;

  // log2(2*Resolution) -- how many of qbit's top bits identify a slot.
  static constexpr uint32_t k =
      static_cast<uint32_t>(std::bit_width(2u * Resolution - 1));
  static_assert(qbit > k,
                "Resolution has more slots than this Torus's qbit can "
                "distinguish");

  // The Torus value naming slot `index` (index must be < Resolution).
  // `index` accepts bool implicitly, so a caller building a Dial<2, ...>
  // can write at(true)/at(false) directly instead of at(1)/at(0).
  static constexpr Torus at(uint32_t index) {
    assert(index < Resolution);
    return Torus(index, 2u * Resolution);
  }

  // The slot nearest `t`, tolerating up to half a slot's worth of noise --
  // the same margin tfhe/gate/hom_and_test.cpp's own correctness check
  // (decode_margin = mu / 2) already judges a correct decode against.
  //
  // Exact integer arithmetic throughout: the same narrowing "round to
  // nearest via a half-ULP offset, then shift" trick
  // tfhe/math/modswitch.hpp's own mod_switch already uses to move a
  // Torus's raw value down to a smaller power-of-two modulus, applied
  // here to shrink it to 2*Resolution slots instead. Going through double
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
    // bit carried past the top is an exact multiple of 2^k that
    // slot_mask discards next, same as mod_switch's own reasoning.
    Word idx2R = static_cast<Word>((raw + half) >> drop) & slot_mask;
    return static_cast<uint32_t>(idx2R) % Resolution;
  }

  // Half a slot's width: the largest noise magnitude decode() still
  // resolves to the intended slot. Equivalent to hom_and_test.cpp's own
  // hand-written decode_margin (double(rTorus(1u, 4u)) / 2) for
  // Resolution=2; named here so a caller never has to re-derive it.
  static constexpr Torus margin() { return Torus(1u, 4u * Resolution); }
};

#endif  // TFHE_DIAL_HPP
