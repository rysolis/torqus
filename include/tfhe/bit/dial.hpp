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

  using Word = typename Torus::raw_value_type;
  static constexpr uint32_t qbit = Torus::qbit;
  static constexpr uint32_t resolution = Resolution;

  // A power-of-two Resolution divides 2^qbit evenly, so its slot
  // boundaries land on exact bit positions and index() can use a plain
  // shift (see the constructor/index()/margin() comments below); any
  // other Resolution needs a genuine multiply/round, see index().
  static constexpr bool is_power_of_two = (Resolution & (Resolution - 1)) == 0;

  // log2(Resolution), rounded up -- how many bits are needed to name one
  // of Resolution slots. Only doubles as "how many of qbit's top bits
  // identify a slot" when is_power_of_two.
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
  // noise (margin()), in exact integer arithmetic throughout -- double
  // would lose bits once qbit exceeds its 52-bit mantissa (e.g.
  // ModTorus<64, uint64_t>).
  constexpr uint32_t index() const {
    Word raw = static_cast<Word>(value_.value());

    if constexpr (is_power_of_two) {
      // Same round-to-nearest-via-half-ULP-then-shift trick as
      // modswitch.hpp's mod_switch: Resolution's slots land on exact bit
      // positions, so rounding is a plain shift.
      constexpr uint32_t drop = qbit - k;
      constexpr Word half = Word{1} << (drop - 1);
      constexpr Word slot_mask = static_cast<Word>((Word{1} << k) - 1);

      // raw + half can wrap when qbit is Word's full width (ModTorus's
      // mod-0 sentinel) -- harmless, slot_mask discards the extra bit next.
      Word idx = static_cast<Word>((raw + half) >> drop) & slot_mask;
      return static_cast<uint32_t>(idx);
    } else if constexpr (qbit <= 32) {
      // Resolution doesn't divide 2^qbit evenly, so slots don't land on
      // exact bit positions -- round to the nearest of Resolution
      // evenly-spaced slots via floor((raw * Resolution + 2^(qbit-1)) /
      // 2^qbit) mod Resolution: the same half-ULP rounding as above, just
      // as a genuine multiply since there's no shared bit boundary to
      // shift across. raw and Resolution both fit in 32 bits here, so
      // their product plus the rounding half always fits in uint64_t.
      constexpr uint64_t half = uint64_t{1} << (qbit - 1);
      uint64_t idx = (static_cast<uint64_t>(raw) * Resolution + half) >> qbit;
      return static_cast<uint32_t>(idx % Resolution);
    } else {
      // qbit > 32 (so qbit <= 64, Word's own limit -- see the QBit <=
      // digits<Word> static_assert on ModTorus): raw alone can already
      // need every bit of a 64-bit Word, so raw * Resolution (Resolution
      // being up to 32 bits) can need up to 96 bits. __int128 would cover
      // that, but it isn't ISO C++ (rejected under -Wpedantic on GCC), so
      // the 96-bit product is built from two 64-bit halves by hand,
      // schoolbook-multiplication style, same idea as ModTorus's own
      // bit-serial (raw, resolution) constructor avoiding a wide type.
      static_assert(qbit <= 64, "Word wider than 64 bits isn't supported");
      constexpr uint64_t half = uint64_t{1} << (qbit - 1);

      // raw * Resolution == (raw_hi * Resolution) << 32 + raw_lo * Resolution,
      // i.e. a << 32 + b below -- computed as a (hi, low) pair of 64-bit
      // words rather than a single wider type.
      uint64_t raw_hi = raw >> 32;
      uint64_t raw_lo = raw & 0xFFFFFFFFu;
      uint64_t a = raw_hi * Resolution;
      uint64_t b = raw_lo * Resolution;

      // a << 32 truncates to (a's low 32 bits) << 32 in a 64-bit register
      // -- exactly A_lo * 2^32 from the derivation above -- while a's own
      // high 32 bits (a >> 32) carry into the pair's high word.
      uint64_t a_lo = a << 32;
      uint64_t low = a_lo + b;
      uint64_t hi = (a >> 32) + (low < a_lo ? 1 : 0);

      // Fold the rounding half into the same (hi, low) pair.
      uint64_t low_rounded = low + half;
      hi += (low_rounded < low) ? 1 : 0;

      uint64_t idx;
      if constexpr (qbit == 64) {
        // low_rounded contributes nothing past bit 64 here -- shifting it
        // right by a full 64 bits would be undefined behavior, so this
        // case only needs the high word.
        idx = hi;
      } else {
        idx = (low_rounded >> qbit) | (hi << (64 - qbit));
      }
      return static_cast<uint32_t>(idx % Resolution);
    }
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
