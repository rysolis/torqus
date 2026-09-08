// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIPHER_CIPHER_HPP
#define TFHE_CIPHER_CIPHER_HPP

#include <cstdint>
#include <utility>
#include <variant>

#include "tfhe/operation/leveled/key_switch.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Cipher<Lwe, Rlwe> is a ciphertext that hides whether it is currently
// Lwe-shaped (TLWE<Torus, n>, ready to feed straight into a gate) or
// Rlwe-shaped (TLWE<rTorus, N>, what every gate call returns). It carries
// no notion of resolution or value on its own -- whether it reads as a
// boolean or as one of many values is entirely up to which
// Dial<Resolution, Torus> decodes it; a Cipher decoded through Dial<4,
// Torus> is a boolean, the same Cipher decoded through some other
// resolution isn't.
// See tfhe/gate/hom_and.hpp: HomAnd/HomOr/HomAndNot/HomXor need both
// operands already Lwe-shaped; Relay::materialize() (tfhe/circuit/relay.hpp)
// converts one that isn't.
template <typename Lwe, typename Rlwe>
class Cipher {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  Cipher(TLWE<Torus, n> ciphertext) : state_(std::move(ciphertext)) {}
  Cipher(TLWE<rTorus, N> ciphertext) : state_(std::move(ciphertext)) {}

  // True once this is Lwe-shaped -- safe to read via ready()
  // without a materialize() first.
  bool is_ready() const {
    return std::holds_alternative<TLWE<Torus, n>>(state_);
  }

  // Valid only when is_ready(). The && overload moves out of a temporary
  // Cipher instead of copying (e.g. `lift.encrypt(v).ready()`).
  const TLWE<Torus, n>& ready() const& {
    return std::get<TLWE<Torus, n>>(state_);
  }
  TLWE<Torus, n>&& ready() && {
    return std::get<TLWE<Torus, n>>(std::move(state_));
  }

  // Valid only when !is_ready(). Still decryptable directly under the
  // Rlwe secret's own coefficients (see Cryptor's sample-extracted
  // decrypt overload) -- a circuit's final output can read this straight
  // off without paying for a materialize() it doesn't need. The &&
  // overload moves out of a temporary Cipher instead of copying.
  const TLWE<rTorus, N>& pending() const& {
    return std::get<TLWE<rTorus, N>>(state_);
  }
  TLWE<rTorus, N>&& pending() && {
    return std::get<TLWE<rTorus, N>>(std::move(state_));
  }

  // Converts in place to Lwe-shaped -- a no-op if already is_ready().
  // Prefer calling this through Relay::materialize(), which deduces Kst
  // from the Relay's own type.
  template <typename Kst>
  void materialize(const KeySwitchKey<Torus, n, Kst::t, N>& ksk) {
    if (is_ready()) return;
    state_ = tfhe::leveled::KeySwitch<ExtractedLwe<Rlwe>, Lwe, Kst>::exec_impl(
        pending(), ksk);
  }

 private:
  std::variant<TLWE<Torus, n>, TLWE<rTorus, N>> state_;
};

#endif  // TFHE_CIPHER_CIPHER_HPP
