// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_PUBLIC_RUNTIME_HPP
#define TFHE_PUBLIC_RUNTIME_HPP

#include <random>
#include <utility>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/public_key.hpp"

// PublicRuntime pairs a PublicKey with the randomness it needs to encrypt --
// the public-key counterpart to Runtime, which instead pairs a secret with
// the randomness it needs to encrypt/decrypt. Unlike Runtime, it never
// touches the secret its PublicKey was generated from (see public_key.hpp),
// so whoever holds one can encrypt without being trusted with that secret.
// Both the key and the engine are owned by value, so a PublicRuntime can be
// built wherever it's used (e.g. by a Client that received its PublicKey
// over the wire from Coordinator) with its own local randomness, rather than
// sharing state with whoever generated the key.
template <typename Params, uint32_t PkSamples>
  requires tlwe_concept<Params>
class PublicRuntime {
 public:
  using params_type = Params;
  using Torus = typename Params::torus_type;
  static constexpr uint32_t n = Params::n;
  // The concrete engine type PublicKey encryption ultimately binds to (see
  // Cryptor::Engine, which every secret-key Runtime already fixes to this
  // same type).
  using Engine = std::mt19937;

  PublicRuntime(PublicKey<Torus, n, PkSamples> pk, Engine eng)
      : pk_(std::move(pk)), eng_(std::move(eng)) {}

  TLWE<Torus, n> encrypt(const Torus& pt) {
    return public_key::encrypt(pk_, eng_, pt);
  }

 private:
  PublicKey<Torus, n, PkSamples> pk_;
  Engine eng_;
};

#endif  // TFHE_PUBLIC_RUNTIME_HPP
