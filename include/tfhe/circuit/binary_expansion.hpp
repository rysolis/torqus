// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <cstdint>

#include "algebra/vector.hpp"

#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/operation/leveled/key_switch.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) input bit-ciphertexts, each Lwe-shaped -- the same
// shape HomAnd/HomAndNot take. Every HomAnd/HomAndNot call returns an
// Rlwe-domain result (see HomAnd), so this loop KeySwitches it (via
// ExtractedLwe<Rlwe>, the Lwe-shaped view of Rlwe's own extracted-key
// domain, and ksk) back down to Lwe before feeding it into the next gate;
// only the last gate of each slot's chain is left at Rlwe's dimension, so
// this as a whole has the same Lwe-in/Rlwe-out shape a single gate does.
namespace tfhe::circuit {

template <uint32_t H, typename Lwe, typename Rlwe, typename Decomp,
          typename Kst>
class BinaryExpansion {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  static constexpr uint32_t k = std::bit_width(H - 1);

  static Vector<TLWE<rTorus, N>, H> exec_impl(
      const Vector<TLWE<Torus, n>, k>& v,
      const KeySwitchKey<Torus, n, t, N>& ksk,
      const BootstrapKey<rTorus, N, l, n>& bk) {
    Vector<TLWE<rTorus, N>, H> res;
    for (size_t h = 0; h < H; ++h) {
      TLWE<Torus, n> w;
      w.b() = Torus(1u, 4u);

      TLWE<rTorus, N> gate;
      gate.b() = rTorus(1u, 4u);
      for (size_t i = 0; i < k; ++i) {
        uint32_t bit = (h >> i) & 1u;
        gate = bit ? gate::HomAnd<Lwe, Rlwe, Decomp>::exec_impl(w, v[i], bk)
                   : gate::HomAndNot<Lwe, Rlwe, Decomp>::exec_impl(w, v[i],
                                                                    bk);
        if (i + 1 < k) {
          w = leveled::KeySwitch<ExtractedLwe<Rlwe>, Lwe, Kst>::exec_impl(
              gate, ksk);
        }
      }
      res[h] = std::move(gate);
    }
    return res;
  }
};

}  // namespace tfhe::circuit

#endif