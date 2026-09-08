// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_RELAY_HPP
#define TFHE_CIRCUIT_RELAY_HPP

#include <cstdint>

#include "tfhe/cipher/cipher.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Relay<Lwe, Rlwe, Kst> holds a pointer to the raw KeySwitchKey needed to
// materialize a Cipher -- converting a gate's Rlwe-shaped result back
// down to Lwe-shaped so it can feed into another gate call. A caller just
// keeps its own KeySwitchKey<Torus, n, t, N> (e.g. as a member) and passes
// it straight through.
//
// The referenced key must outlive this Relay.
template <typename Lwe, typename Rlwe, typename Kst>
class Relay {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t t = Kst::t;

  Relay() = default;
  explicit Relay(const KeySwitchKey<Torus, n, t, N>& ksk) : ksk_(&ksk) {}

  // Converts `bit` in place to Lwe-shaped -- a no-op if already
  // bit.is_ready().
  void materialize(Cipher<Lwe, Rlwe>& bit) const {
    bit.template materialize<Kst>(*ksk_);
  }

 private:
  const KeySwitchKey<Torus, n, t, N>* ksk_;
};

#endif  // TFHE_CIRCUIT_RELAY_HPP
