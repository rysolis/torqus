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

namespace {

template <kst_concept params, torus_type Torus>
UInt decompose(const Torus& v, size_t i) {
  static constexpr uint32_t Bbit = std::bit_width(params::K - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  UInt::raw_value_type w =
      UInt::raw_value_type(static_cast<Torus::raw_value_type>(v));
  UInt::raw_value_type tmp = (w >> shift) & (params::K - 1);
  return UInt(tmp);
}

template <kst_concept params, torus_type Torus>
Torus reconstruct(const Vector<Poly<UInt, params::N>, params::t>& repr,
                  size_t j) {
  static constexpr uint32_t Bbit = std::bit_width(params::K - 1);
  typename Torus::raw_value_type m = 0;
  for (size_t i = 0; i < params::t; ++i) {
    size_t shift = Torus::qbit - (Bbit * (i + 1));
    assert(shift <= (Torus::qbit - Bbit));
    UInt::raw_value_type v = static_cast<UInt::raw_value_type>(repr[i][j]);
    m |= v << shift;
  }
  return Torus(m);
}

}  // namespace

template <typename SrcLwe, typename DstLwe, typename Kst>
  requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
class KeySwitch {
 public:
  using dTorus = typename DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;

  using sTorus = typename SrcLwe::torus_type;
  static constexpr uint32_t N = SrcLwe::n;

  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  inline static TLWE<dTorus, n> exec(const TLWE<sTorus, N>& src,
                                     const KeySwitchKey<dTorus, n, t, N>& KSK) {
    TLWE<dTorus, n> dst;
    dst.b() = dTorus(static_cast<sTorus::raw_value_type>(src.b()));
    for (size_t i = 0; i < N; ++i) {
      dTorus ai = src.a()[i];
      for (size_t j = 0; j < t; ++j) {
        UInt d = decompose<Kst>(ai, j);
        dst -= d * KSK[i][j];
      }
    }
    return dst;
  }
};

namespace keyswitch_key {

template <typename SrcLwe, typename DstLwe, typename Kst, typename Cryptor,
          typename Holder>
  requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
KeySwitchKey<typename DstLwe::torus_type, DstLwe::n, Kst::t,
             SrcLwe::n> static generate(Cryptor& cryptor,
                                        const Holder& holder) {
  static constexpr uint32_t N = SrcLwe::n;
  using Torus = DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;
  static constexpr uint32_t t = Kst::t;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t Kbit = std::bit_width(K - 1);

  KeySwitchKey<Torus, n, t, N> KSK;

  for (uint32_t i = 0; i < N; ++i) {
    Torus s(holder.secret_ptr()[i]);
    for (size_t j = 0; j < t; ++j) {
      typename Torus::raw_value_type tmp =
          static_cast<typename Torus::raw_value_type>(s)
          << (Torus::qbit - Kbit * (j + 1));
      KSK[i][j] = cryptor.encrypt(Torus(tmp));
    }
  }

  return KSK;
}

}  // namespace keyswitch_key

#endif
