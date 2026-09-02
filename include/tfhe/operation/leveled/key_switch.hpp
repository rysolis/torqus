// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_KEY_SWITCH_HPP
#define TFHE_KEY_SWITCH_HPP

#include <bit>
#include <cstdint>

#include "primitive/concept/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

namespace tfhe::leveled {

namespace {

// decompose/reconstruct below shift in TorusWord (Torus's own
// raw_value_type), not UInt::raw_value_type: once Torus is wider than
// UInt (see primitive/word.hpp), a shift amount can exceed UInt's own bit
// width. Only the final digit (< Params::K, always small) narrows down
// to UInt.

template <kst_concept Params, torus_concept Torus>
UInt decompose(const Torus& v, size_t i) {
  using TorusWord = typename Torus::raw_value_type;
  static constexpr uint32_t Bbit = std::bit_width(Params::K - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  TorusWord round = 0;
  if constexpr (Torus::qbit > Bbit * Params::t) {
    round = TorusWord{1} << (Torus::qbit - Bbit * Params::t - 1);
  }
  TorusWord w = static_cast<TorusWord>(v) + round;
  UInt::raw_value_type tmp =
      static_cast<UInt::raw_value_type>((w >> shift) & (Params::K - 1));
  return UInt(tmp);
}

template <kst_concept Params, torus_concept Torus>
Torus reconstruct(const Vector<Poly<UInt, Params::N>, Params::t>& repr,
                  size_t j) {
  using TorusWord = typename Torus::raw_value_type;
  static constexpr uint32_t Bbit = std::bit_width(Params::K - 1);
  TorusWord m = 0;
  for (size_t i = 0; i < Params::t; ++i) {
    size_t shift = Torus::qbit - (Bbit * (i + 1));
    assert(shift <= (Torus::qbit - Bbit));
    TorusWord v =
        static_cast<TorusWord>(static_cast<UInt::raw_value_type>(repr[i][j]));
    m |= v << shift;
  }
  return Torus(m);
}

}  // namespace

template <typename SrcLwe, typename DstLwe, typename Kst>
  requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
class KeySwitch {
 public:
  using DstTorus = typename DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;

  using SrcTorus = typename SrcLwe::torus_type;

  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  static TLWE<DstTorus, n> exec_impl(
      const TLWE<SrcTorus, SrcLwe::n>& src,
      const KeySwitchKey<DstTorus, n, t, SrcLwe::n>& ksk) {
    TLWE<DstTorus, n> dst;
    dst.b() = DstTorus(static_cast<SrcTorus::raw_value_type>(src.b()));
    for (size_t i = 0; i < SrcLwe::n; ++i) {
      DstTorus ai = src.a()[i];
      for (size_t j = 0; j < t; ++j) {
        UInt d = decompose<Kst>(ai, j);
        dst -= d * ksk[i][j];
      }
    }
    return dst;
  }
};

}  // namespace tfhe::leveled

#endif
