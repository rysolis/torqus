// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_PUBLIC_KEY_HPP
#define TFHE_PUBLIC_KEY_HPP

#include <cstdint>
#include <random>
#include <vector>

// A PublicKey is PkSamples independent fresh encryptions of zero under one
// secret, generated once by whoever holds that secret (see
// Runtime::generate_public_key) -- Ciphertext is whichever shape those
// samples are (TLWE for an Lwe-side secret, TRLWE/TRGSW for an Rlwe-side
// one). Anyone holding just the PublicKey can then encrypt a message under
// that same secret without ever seeing it: sum a random subset of the
// samples (still an encryption of zero, since that's closed under
// addition for all of TLWE/TRLWE/TRGSW) and fold the message in -- see
// public_key::encrypt below for the TLWE/TRLWE case (message folds
// straight into the result's phase) and trgsw::public_encrypt
// (cryptor/detail/trgsw_cryptor.hpp) for TRGSW's own gadget-scaled fold.
template <typename Ciphertext, uint32_t PkSamples_>
class PublicKey {
 public:
  static constexpr uint32_t PkSamples = PkSamples_;

  PublicKey() = default;

  Ciphertext& operator[](size_t idx) noexcept { return samples_[idx]; }
  const Ciphertext& operator[](size_t idx) const noexcept {
    return samples_[idx];
  }

 private:
  // Ciphertext isn't a numeric primitive Vector<T,Size> is built for -- see
  // trgsw.hpp's own comment.
  std::vector<Ciphertext> samples_ = std::vector<Ciphertext>(PkSamples);
};

namespace public_key {

// Encrypts `message` under `pk` -- no secret required. A random subset of
// pk's zero-samples is summed (still an encryption of zero) and `message`
// is folded straight into the result's phase via Ciphertext's own `.b()`,
// yielding a fresh encryption of `message` under the same secret pk was
// generated from. Works unchanged for both TLWE (message: Torus) and TRLWE
// (message: Poly<Torus,N>) -- TRGSW doesn't fit this shape (its message
// isn't a single `.b()` fold, see trgsw::public_encrypt instead).
template <typename Ciphertext, uint32_t PkSamples, typename Engine,
          typename Plaintext>
  requires std::uniform_random_bit_generator<Engine>
Ciphertext encrypt(const PublicKey<Ciphertext, PkSamples>& pk, Engine& eng,
                   const Plaintext& message) {
  std::bernoulli_distribution dist(0.5);
  Ciphertext ct;
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
