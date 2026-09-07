// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_RESLOT_HPP
#define TFHE_CIRCUIT_RESLOT_HPP

#include <cstdint>
#include <utility>

#include "tfhe/bit/bit.hpp"
#include "tfhe/circuit/circuit.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

// Wraps Circuit::Reslot<InResolution, OutResolution> plus the
// Relay::materialize() a caller almost always wants right after it -- a
// ciphertext arriving at a foreign 1/InResolution step (e.g. a ballot bit
// encoded outside this library's own gate suite) goes in and a plain
// Lwe-shaped TLWE at this circuit's 1/OutResolution step comes out, with
// no Bit unwrapping at the call site.
//
// Holds the Circuit/Relay by reference, not by value -- a caller that also
// needs the underlying And/Or/AndNot/Xor/Reslot keeps its own Circuit and
// Relay and layers this (and any other circuit:: wrapper) on top, rather
// than duplicating the BootstrapKey/KeySwitchKey to give each wrapper its
// own copy. The referenced Circuit/Relay must outlive this Reslot.
namespace tfhe::circuit {

template <uint32_t InResolution, uint32_t OutResolution, typename Lwe,
          typename Rlwe, typename Decomp, typename Kst>
class Reslot {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  Reslot(const Circuit<Lwe, Rlwe, Decomp>& circuit,
         const Relay<Lwe, Rlwe, Kst>& relay)
      : circuit_(&circuit), relay_(&relay) {}

  TLWE<Torus, n> exec(const TLWE<Torus, n>& c) const {
    Bit<Lwe, Rlwe> bit = circuit_->template Reslot<InResolution, OutResolution>(
        Bit<Lwe, Rlwe>(c));
    relay_->materialize(bit);
    return std::move(bit).ready();
  }

 private:
  const Circuit<Lwe, Rlwe, Decomp>* circuit_;
  const Relay<Lwe, Rlwe, Kst>* relay_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_RESLOT_HPP
