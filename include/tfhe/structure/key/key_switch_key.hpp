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

 private:
  Vector<Vector<TLWE<Torus, n>, t>, N> ks_;
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
