// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <array>
#include <cstdint>
#include <utility>

#include "algebra/vector.hpp"

#include "tfhe/bit.hpp"
#include "tfhe/params.hpp"
#include "tfhe/scope.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) Lwe-shaped input bit-ciphertexts. Each slot chains
// HomAnd/HomAndNot through Bit<Lwe, Rlwe>, materializing between steps
// (via this instance's own Relay/Circuit); only the last step per slot
// stays Rlwe-shaped, so the whole thing has the same Lwe-in/Rlwe-out
// shape a single gate does. Output is Bit, not raw TLWE, so chaining this
// circuit's result into another Circuit call needs no manual rewrapping
// (Vector<T,Size> itself can't hold Bit -- it stores element types as a
// flat raw_value_type buffer, which Bit's std::variant state doesn't fit).
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

  // One slot of the one-hot output. The k-step gate chain is sequential
  // (each step materializes the previous Bit before the next gate call),
  // but slots are independent -- farm them across threads instead of
  // calling exec_impl directly if needed.
  Bit<Lwe, Rlwe> exec_slot_impl(uint32_t h,
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
    return acc;
  }

  std::array<Bit<Lwe, Rlwe>, H> exec_impl(
      const Vector<TLWE<Torus, n>, k>& v) const {
    return exec_impl_impl(v, std::make_index_sequence<H>{});
  }

 private:
  template <size_t... Hs>
  std::array<Bit<Lwe, Rlwe>, H> exec_impl_impl(
      const Vector<TLWE<Torus, n>, k>& v, std::index_sequence<Hs...>) const {
    return {exec_slot_impl(static_cast<uint32_t>(Hs), v)...};
  }

  Circuit<Lwe, Rlwe, Decomp> circuit_;
  Relay<Lwe, Rlwe, Kst> relay_;
};

}  // namespace tfhe::circuit

#endif
