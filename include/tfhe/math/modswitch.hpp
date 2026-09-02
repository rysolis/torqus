// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_MODSWITCH_HPP
#define TFHE_MODSWITCH_HPP

#include <bit>
#include <cstdint>
#include <limits>

#include "primitive/modint.hpp"

namespace {
template <uint64_t N>
concept power_of_two_concept = (N & (N - 1)) == 0;
}

// Both moduli are powers of two for every param set this library actually
// builds (M is always 2 * a ring dimension, N is always a Torus's own
// 2^qbit), so M/N (or N/M) is itself an exact power of two and the whole
// rescale-and-round is a plain shift -- no multiply, let alone a widening
// one, is needed. (A mod_switch between two moduli where M were not a
// power of two would need the full t*M product, which -- once t spans a
// full 64-bit Word -- doesn't fit in 64 bits; that version of this
// function used a 128-bit widening multiply, but there is no caller in
// this codebase that needs it.)
template <uint64_t M, uint64_t N, typename Word>
  requires power_of_two_concept<N> && power_of_two_concept<M>
constexpr ModInt<M> mod_switch(const ModInt<N, Word>& t) {
  using DstWord = typename ModInt<M>::raw_value_type;

  // N == 0 / M == 0 mean "no reduction", i.e. that side's true modulus is
  // 2^(its own Word's width) -- see ModInt's own mod == 0 convention.
  constexpr uint64_t n_bits =
      N == 0 ? static_cast<uint64_t>(std::numeric_limits<Word>::digits)
             : std::bit_width(N - 1);
  constexpr uint64_t m_bits =
      M == 0 ? static_cast<uint64_t>(std::numeric_limits<DstWord>::digits)
             : std::bit_width(M - 1);

  const uint64_t raw = static_cast<uint64_t>(t.value());

  if constexpr (m_bits >= n_bits) {
    // Widening (or exact, if equal): M/N is an integer power of two, so
    // t*(M/N) is exact -- no rounding, and raw < N leaves raw << (m_bits
    // - n_bits) < M, so this can't overflow uint64_t either.
    return ModInt<M>(raw << (m_bits - n_bits));
  } else {
    // Narrowing: round-to-nearest via a half-ULP offset before dropping
    // the low (n_bits - m_bits) bits. raw + half can overflow uint64_t
    // only when N is itself the "full Word width" sentinel above -- and
    // in that case the overflow is exactly one extra multiple of 2^m_bits
    // (i.e. of M) carried past bit 63, which ModInt<M>'s own reduction
    // below discards anyway, so letting it wrap here needs no extra
    // handling.
    constexpr uint64_t drop = n_bits - m_bits;
    constexpr uint64_t half = uint64_t{1} << (drop - 1);
    return ModInt<M>((raw + half) >> drop);
  }
}

#endif  // TFHE_MODSWITCH_HPP
