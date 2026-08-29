// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CRYPTOR_HPP
#define TFHE_CRYPTOR_HPP

#include <random>
#include <utility>

#include "primitive/concept/torus.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/cryptor/detail/tlwe_cryptor.hpp"
#include "tfhe/cryptor/detail/trgsw_cryptor.hpp"
#include "tfhe/cryptor/detail/trlwe_cryptor.hpp"

template <typename Params, typename... Feature>
class Cryptor {
 public:
  using Engine = std::mt19937;
  using params_type = Params;
  using Torus = Params::torus_type;

  Cryptor(std::shared_ptr<UInt::raw_value_type[]> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {}

  template <typename Plaintext>
    requires(tlwe_concept<Params> && std::same_as<Plaintext, Torus>)
  auto encrypt(const Plaintext& pt) {
    return tlwe::encrypt<Params>(secret_, eng_, pt);
  }

  template <typename Plaintext>
    requires(trlwe_concept<Params> &&
             std::same_as<Plaintext, Poly<Torus, Params::N>>)
  auto encrypt(const Plaintext& pt) {
    return trlwe::encrypt<Params>(secret_, eng_, pt);
  }

  template <typename Plaintext>
    requires(trlwe_concept<Params> && decompose_concept<Params> &&
             std::same_as<Plaintext, Poly<UInt, Params::N>>)
  auto encrypt(const Plaintext& pt) {
    return trgsw::encrypt<Params>(secret_, eng_, pt);
  }

  template <typename Ciphertext>
    requires(tlwe_concept<Params> &&
             std::same_as<Ciphertext, TLWE<Torus, Params::n>>)
  auto decrypt(const Ciphertext& ct) {
    return tlwe::decrypt<Params>(secret_, ct);
  }

  template <typename Ciphertext>
    requires(trlwe_concept<Params> &&
             std::same_as<Ciphertext, TRLWE<Torus, Params::N>>)
  auto decrypt(const Ciphertext& ct) {
    return trlwe::decrypt<Params>(secret_, ct);
  }

  // Decrypts a TLWE ciphertext sample-extracted from an RLWE ciphertext,
  // still under the RLWE secret's own coefficients.
  template <typename Ciphertext>
    requires(trlwe_concept<Params> &&
             std::same_as<Ciphertext, TLWE<Torus, Params::N>>)
  auto decrypt(const Ciphertext& ct) {
    return trlwe::decrypt<Params>(secret_, ct);
  }

 private:
  std::shared_ptr<UInt::raw_value_type[]> secret_;
  std::reference_wrapper<Engine> eng_;
};

#endif  // TFHE_CRYPTOR_HPP