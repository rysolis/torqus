// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_PUBLIC_KEY_HPP
#define TFHE_PUBLIC_KEY_HPP

#include <cstdint>
#include <random>

#include "primitive/concept/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/tlwe.hpp"

// A PublicKey is PkSamples independent TLWE(0) samples under one secret --
// fresh encryptions of zero, generated once by whoever holds that secret
// (see Runtime::generate_public_key). Anyone holding just the PublicKey can
// then encrypt a message under that same secret without ever seeing it: sum
// a random subset of the samples (still TLWE(0), since TLWE(0) is closed
// under addition) and add the message to the result's phase -- see
// public_key::encrypt below.
template <torus_concept Torus, uint32_t n_, uint32_t PkSamples_>
class PublicKey {
 public:
  static constexpr uint32_t n = n_;
  static constexpr uint32_t PkSamples = PkSamples_;

  PublicKey() = default;

  TLWE<Torus, n>& operator[](size_t idx) noexcept { return samples_[idx]; }
  const TLWE<Torus, n>& operator[](size_t idx) const noexcept {
    return samples_[idx];
  }

 private:
  Vector<TLWE<Torus, n>, PkSamples> samples_;
};

namespace public_key {

// Encrypts `message` under `pk` -- no secret required. A random subset of
// pk's TLWE(0) samples is summed (still TLWE(0)) and `message` is folded
// into the result's phase, yielding a fresh TLWE(message) under the same
// secret pk was generated from.
template <typename Torus, uint32_t n, uint32_t PkSamples, typename Engine>
  requires std::uniform_random_bit_generator<Engine>
TLWE<Torus, n> encrypt(const PublicKey<Torus, n, PkSamples>& pk, Engine& eng,
                       const Torus& message) {
  std::bernoulli_distribution dist(0.5);
  TLWE<Torus, n> ct;
  for (uint32_t i = 0; i < PkSamples; ++i) {
    if (dist(eng)) {
      ct += pk[i];
    }
  }
  ct.b() += message;
  return ct;
}

}  // namespace public_key

#endif  // TFHE_PUBLIC_KEY_HPP
