// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <cstdint>

#include "algebra/vector.hpp"

#include "tfhe/bit.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) input bit-ciphertexts, each Lwe-shaped -- the same
// shape HomAnd/HomAndNot take. Every HomAnd/HomAndNot call returns an
// Rlwe-domain result (see HomAnd), so this loop chains them through
// Bit<Lwe, Rlwe>, which materializes (KeySwitches) each step's result back
// down to Lwe before feeding it into the next gate; only the last gate of
// each slot's chain is left at Rlwe's dimension, so this as a whole has
// the same Lwe-in/Rlwe-out shape a single gate does.
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

  // Computes one slot of the one-hot output. Slots only share read-only
  // inputs (v, ksk, bk) -- the k-step gate chain is sequential within a
  // slot (each step materializes the previous gate's Bit before feeding
  // it to the next), but slots don't depend on each other, so callers with
  // many slots can farm this out across threads instead of calling
  // exec_impl directly.
  static TLWE<rTorus, N> exec_slot_impl(
      uint32_t h, const Vector<TLWE<Torus, n>, k>& v,
      const KeySwitchKey<Torus, n, t, N>& ksk,
      const BootstrapKey<rTorus, N, l, n>& bk) {
    TLWE<Torus, n> w;
    w.b() = Torus(1u, 4u);
    Bit<Lwe, Rlwe> gate = w;

    for (size_t i = 0; i < k; ++i) {
      uint32_t bit = (h >> i) & 1u;
      Bit<Lwe, Rlwe> vi = v[i];
      gate = bit ? tfhe::bit::And<Kst, Decomp>(gate, vi, bk, ksk)
                 : tfhe::bit::AndNot<Kst, Decomp>(gate, vi, bk, ksk);
    }
    return gate.pending_ciphertext();
  }

  static Vector<TLWE<rTorus, N>, H> exec_impl(
      const Vector<TLWE<Torus, n>, k>& v,
      const KeySwitchKey<Torus, n, t, N>& ksk,
      const BootstrapKey<rTorus, N, l, n>& bk) {
    Vector<TLWE<rTorus, N>, H> res;
    for (uint32_t h = 0; h < H; ++h) {
      res[h] = exec_slot_impl(h, v, ksk, bk);
    }
    return res;
  }
};

}  // namespace tfhe::circuit

#endif