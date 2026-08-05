// Copyright 2026 EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_KEY_SWITCH_KEY_HPP
#define TFHE_KEY_SWITCH_KEY_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/tlwe.hpp"

template <torus_type Torus, uint32_t n, uint32_t t, uint32_t N>
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
  Vector<Vector<TLWE<Torus, n>, t>, N> ks_;
};

namespace keyswitch_key {

template <typename SrcLwe, typename DstLwe, typename Kst, typename Cryptor>
  requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
KeySwitchKey<typename DstLwe::torus_type, DstLwe::n, Kst::t,
             SrcLwe::n> static generate(Cryptor& cryptor,
                                        const UInt::raw_value_type* s) {
  static constexpr uint32_t N = SrcLwe::n;
  using Torus = DstLwe::torus_type;
  static constexpr uint32_t n = DstLwe::n;
  static constexpr uint32_t t = Kst::t;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t Kbit = std::bit_width(K - 1);

  KeySwitchKey<Torus, n, t, N> KSK;
  for (uint32_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < t; ++j) {
      typename Torus::raw_value_type v = s[i] << (Torus::qbit - Kbit * (j + 1));
      KSK[i][j] = cryptor.encrypt(Torus(v));
    }
  }

  return KSK;
}

}  // namespace keyswitch_key

#endif
