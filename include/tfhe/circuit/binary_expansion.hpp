// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "tfhe/bit/bit.hpp"
#include "tfhe/bit/scope.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) Lwe-shaped input bit-ciphertexts. Each slot chains
// HomAnd/HomAndNot through Bit<Lwe, Rlwe>, materializing between steps
// (via this instance's own Relay/Circuit); only the last step per slot
// stays Rlwe-shaped, so the whole thing has the same Lwe-in/Rlwe-out
// shape a single gate does. Output is Bit, not raw TLWE, so chaining this
// circuit's result into another Circuit call needs no manual rewrapping
// (Vector<T,Size> itself can't hold Bit -- it stores element types as a
// flat raw_value_type buffer, which Bit's std::variant state doesn't fit;
// TLWE input is std::vector for the same reason -- neither is the
// numeric-primitive Vector<T,Size> is built for).
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
  // calling exec directly if needed.
  Bit<Lwe, Rlwe> exec_slot_impl(uint32_t h,
                                const std::vector<TLWE<Torus, n>>& v) const {
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

  std::array<Bit<Lwe, Rlwe>, H> exec(
      const std::vector<TLWE<Torus, n>>& v) const {
    return exec_impl(v, std::make_index_sequence<H>{});
  }

  // Same computation as exec_slot_impl(h, v), but also materialized back
  // down to Lwe-shaped before returning -- for a caller farming slots
  // across its own thread pool (see exec_slot_impl's own doc comment),
  // this gives the same "no second Relay needed" convenience exec_ready()
  // gives the single-threaded, all-slots-at-once caller.
  TLWE<Torus, n> exec_slot_ready(uint32_t h,
                                 const std::vector<TLWE<Torus, n>>& v) const {
    Bit<Lwe, Rlwe> bit = exec_slot_impl(h, v);
    relay_.materialize(bit);
    return std::move(bit).ready();
  }

  // Same computation as exec(), but each output slot is also materialized
  // back down to Lwe-shaped before returning -- for a caller that just
  // wants a ready-to-use result instead of a Relay::materialize() call per
  // slot (the same exec-plus-materialize convenience tfhe::circuit::Reslot
  // offers over Circuit::Reslot).
  std::array<TLWE<Torus, n>, H> exec_ready(
      const std::vector<TLWE<Torus, n>>& v) const {
    std::array<TLWE<Torus, n>, H> ready;
    for (uint32_t h = 0; h < H; ++h) {
      ready[h] = exec_slot_ready(h, v);
    }
    return ready;
  }

 private:
  template <size_t... Hs>
  std::array<Bit<Lwe, Rlwe>, H> exec_impl(const std::vector<TLWE<Torus, n>>& v,
                                          std::index_sequence<Hs...>) const {
    return {exec_slot_impl(static_cast<uint32_t>(Hs), v)...};
  }

  Circuit<Lwe, Rlwe, Decomp> circuit_;
  Relay<Lwe, Rlwe, Kst> relay_;
};

}  // namespace tfhe::circuit

#endif
