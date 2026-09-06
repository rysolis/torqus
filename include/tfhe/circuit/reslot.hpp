// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_RESLOT_HPP
#define TFHE_CIRCUIT_RESLOT_HPP

#include <cstdint>
#include <utility>

#include "tfhe/bit/bit.hpp"
#include "tfhe/bit/scope.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

// Wraps Circuit::Reslot<InResolution, OutResolution> plus the
// Relay::materialize() a caller almost always wants right after it -- a
// ciphertext arriving at a foreign 1/InResolution step (e.g. a ballot bit
// encoded outside this library's own gate suite) goes in and a plain
// Lwe-shaped TLWE at this circuit's 1/OutResolution step comes out, with
// no Bit unwrapping at the call site.
namespace tfhe::circuit {

template <uint32_t InResolution, uint32_t OutResolution, typename Lwe,
          typename Rlwe, typename Decomp, typename Kst>
class Reslot {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  Reslot(Circuit<Lwe, Rlwe, Decomp> circuit, Relay<Lwe, Rlwe, Kst> relay)
      : circuit_(std::move(circuit)), relay_(std::move(relay)) {}

  TLWE<Torus, n> exec(const TLWE<Torus, n>& c) const {
    Bit<Lwe, Rlwe> bit = circuit_.template Reslot<InResolution, OutResolution>(
        Bit<Lwe, Rlwe>(c));
    relay_.materialize(bit);
    return std::move(bit).ready();
  }

 private:
  Circuit<Lwe, Rlwe, Decomp> circuit_;
  Relay<Lwe, Rlwe, Kst> relay_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_RESLOT_HPP
