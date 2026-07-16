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

template <key_switch_concept params, torus_type Torus>
UInt decompose(const Torus& v, size_t i) {
  static constexpr uint32_t Bbit = std::bit_width(params::K - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  UInt::raw_value_type w =
      UInt::raw_value_type(static_cast<Torus::raw_value_type>(v));
  UInt::raw_value_type tmp = (w >> shift) & (params::K - 1);
  return UInt(tmp);
}

template <key_switch_concept params, torus_type Torus>
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

template <tlwe_concept fparams, tlwe_concept bparams>
  requires key_switch_concept<bparams>
class KeySwitch {
 public:
  using fTorus = typename fparams::torus_type;
  static constexpr uint32_t n = fparams::n;

  using bTorus = typename bparams::torus_type;
  static constexpr uint32_t N = bparams::n;
  static constexpr uint32_t K = bparams::K;
  static constexpr uint32_t t = bparams::t;

  inline static TLWE<fTorus, n> exec(const TLWE<bTorus, N>& src,
                                     const KeySwitchKey<bTorus, n, t, N>& KSK) {
    TLWE<fTorus, n> dst;
    dst.b() = fTorus(static_cast<bTorus::raw_value_type>(src.b()));
    for (size_t i = 0; i < N; ++i) {
      bTorus ai = src.a()[i];
      for (size_t j = 0; j < t; ++j) {
        UInt d = decompose<bparams>(ai, j);
        dst -= d * KSK[i][j];
      }
    }
    return dst;
  }
};

#endif
