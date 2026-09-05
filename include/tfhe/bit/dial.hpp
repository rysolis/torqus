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

// Dial<Resolution, Torus> wraps one Torus value and views it through
// Resolution evenly spaced slots (i/Resolution for i in [0, Resolution)),
// the same way ModTorus wraps a raw integer and views it as a fraction.
// Build one from a slot index (before encrypting) or from an existing
// Torus (e.g. straight out of decrypt(), to read off index()).
//
// Dial has no opinion of its own about noise margins or how many slots a
// scheme actually uses. tfhe/gate/Hom* (HomAnd/HomOr/HomAndNot/HomXor)
// need each message confined under half the circle (they sum two and
// compare against one fixed boundary), so they use Dial<4, Torus> and
// only indices 0/1 (0 and 1/4, matching mu in hom_and.hpp); indices 2/3
// stay unused headroom.
//
// Resolution has no default -- it's a property of the scheme in use, not
// something to fall back on silently.
template <uint32_t Resolution, modtorus_concept Torus>
class Dial {
 public:
  static_assert(Resolution > 0, "Dial needs at least one slot");
  // Every plaintext modulus here is a power of two (see modswitch.hpp);
  // this keeps the constructor/index()/margin() exact integer arithmetic,
  // no double anywhere in this class.
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
  // can write Dial(true)/Dial(false) directly instead of Dial(1)/Dial(0).
  constexpr explicit Dial(uint32_t index) : value_(index, Resolution) {
    assert(index < Resolution);
  }

  // Views an existing Torus value through this Dial's slots -- e.g. a
  // value fresh out of decrypt(), about to be read off via index().
  constexpr explicit Dial(Torus value) : value_(value) {}

  // The raw Torus value this Dial wraps.
  constexpr const Torus& value() const { return value_; }

  // Nearest slot to this Dial's value, tolerant of up to half a slot's
  // noise (margin()). Same round-to-nearest-via-half-ULP-then-shift trick
  // as modswitch.hpp's mod_switch, in exact integer arithmetic -- double
  // would lose bits once qbit exceeds its 52-bit mantissa (e.g.
  // ModTorus<64, uint64_t>).
  constexpr uint32_t index() const {
    constexpr uint32_t drop = qbit - k;
    constexpr Word half = Word{1} << (drop - 1);
    constexpr Word slot_mask = static_cast<Word>((Word{1} << k) - 1);

    Word raw = static_cast<Word>(value_.value());
    // raw + half can wrap when qbit is Word's full width (ModTorus's
    // mod-0 sentinel) -- harmless, slot_mask discards the extra bit next.
    Word idx = static_cast<Word>((raw + half) >> drop) & slot_mask;
    return static_cast<uint32_t>(idx);
  }

  constexpr bool operator==(const Dial& other) const {
    return value_ == other.value_;
  }

  // Half a slot's width: the largest noise magnitude index() still
  // resolves to the intended slot.
  static constexpr Torus margin() { return Torus(1u, 2u * Resolution); }

 private:
  Torus value_;
};

#endif  // TFHE_DIAL_HPP
