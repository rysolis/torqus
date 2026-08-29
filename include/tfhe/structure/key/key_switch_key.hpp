// Copyright 2026 EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_KEY_SWITCH_KEY_HPP
#define TFHE_KEY_SWITCH_KEY_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/tlwe.hpp"

// `m` is the number of rows, i.e. the source LWE dimension being switched
// away from (SrcLwe::n) -- deliberately not named `N`, which elsewhere in
// this library always denotes an RLWE ring dimension.
template <torus_concept Torus, uint32_t n, uint32_t t, uint32_t m>
class KeySwitchKey {
 public:
  KeySwitchKey() = default;
  Vector<TLWE<Torus, n>, t>& operator[](size_t idx) noexcept {
    return ks_[idx];
  }
  const Vector<TLWE<Torus, n>, t>& operator[](size_t idx) const noexcept {
    return ks_[idx];
  }

  const void* identity() const noexcept { return ks_[0][0].identity(); }

 private:
  Vector<Vector<TLWE<Torus, n>, t>, m> ks_;
};

namespace key_switch_key {

template <typename SrcLwe, typename DstLwe, typename Kst, typename Cryptor>
  requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
KeySwitchKey<typename DstLwe::torus_type, DstLwe::n, Kst::t,
             SrcLwe::n> static generate(Cryptor& cryptor,
                                        const UInt::raw_value_type* secret) {
  using Torus = DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;
  static constexpr uint32_t t = Kst::t;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t Kbit = std::bit_width(K - 1);

  KeySwitchKey<Torus, n, t, SrcLwe::n> ksk;
  for (uint32_t i = 0; i < SrcLwe::n; ++i) {
    for (size_t j = 0; j < t; ++j) {
      typename Torus::raw_value_type v = secret[i]
                                         << (Torus::qbit - Kbit * (j + 1));
      ksk[i][j] = cryptor.encrypt(Torus(v));
    }
  }

  return ksk;
}

}  // namespace key_switch_key

#endif
