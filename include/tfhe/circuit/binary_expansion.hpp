// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <cstdint>
#include <utility>

#include "algebra/vector.hpp"

#include "tfhe/bit.hpp"
#include "tfhe/params.hpp"
#include "tfhe/scope.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) input bit-ciphertexts, each Lwe-shaped -- the same
// shape HomAnd/HomAndNot take. Every HomAnd/HomAndNot call returns an
// Rlwe-domain result (see HomAnd), so this loop chains them through
// Bit<Lwe, Rlwe>, materializing (via this instance's own Relay) each
// step's result back down to Lwe before feeding it into the next gate
// (via this instance's own Circuit); only the last gate of each slot's
// chain is left at Rlwe's dimension, so this as a whole has the same
// Lwe-in/Rlwe-out shape a single gate does.
namespace tfhe::circuit {

template <uint32_t H, typename Lwe, typename Rlwe, typename Decomp,
          typename Kst>
class BinaryExpansion {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t k = std::bit_width(H - 1);

  BinaryExpansion(Circuit<Lwe, Rlwe, Decomp> circuit,
                  Relay<Lwe, Rlwe, Kst> relay)
      : circuit_(std::move(circuit)), relay_(std::move(relay)) {}

  // Computes one slot of the one-hot output. Slots only share read-only
  // inputs (v, and this instance's own circuit_/relay_) -- the k-step gate
  // chain is sequential within a slot (each step materializes the
  // previous gate's Bit before feeding it to the next), but slots don't
  // depend on each other, so callers with many slots can farm this out
  // across threads instead of calling exec_impl directly.
  TLWE<rTorus, N> exec_slot_impl(uint32_t h,
                                 const Vector<TLWE<Torus, n>, k>& v) const {
    TLWE<Torus, n> w;
    w.b() = Torus(1u, 4u);
    Bit<Lwe, Rlwe> acc = w;

    for (size_t i = 0; i < k; ++i) {
      uint32_t bit = (h >> i) & 1u;
      Bit<Lwe, Rlwe> vi = v[i];
      relay_.materialize(acc);
      acc = bit ? circuit_.And(acc, vi) : circuit_.AndNot(acc, vi);
    }
    return acc.pending_ciphertext();
  }

  Vector<TLWE<rTorus, N>, H> exec_impl(
      const Vector<TLWE<Torus, n>, k>& v) const {
    Vector<TLWE<rTorus, N>, H> res;
    for (uint32_t h = 0; h < H; ++h) {
      res[h] = exec_slot_impl(h, v);
    }
    return res;
  }

 private:
  Circuit<Lwe, Rlwe, Decomp> circuit_;
  Relay<Lwe, Rlwe, Kst> relay_;
};

}  // namespace tfhe::circuit

#endif
