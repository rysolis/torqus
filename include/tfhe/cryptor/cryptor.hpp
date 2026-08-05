// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CRYPTOR_HPP
#define TFHE_CRYPTOR_HPP

#include <random>

#include "primitive/concept/torus.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/cryptor/tlwe_cryptor.hpp"
#include "tfhe/cryptor/trgsw_cryptor.hpp"
#include "tfhe/cryptor/trlwe_cryptor.hpp"
#include "tfhe/utility/always_false.hpp"

template <typename T>
struct is_tlwe : std::false_type {};

template <torus_type Torus, uint32_t n>
struct is_tlwe<TLWE<Torus, n>> : std::true_type {};

template <typename T>
concept tlwe_ciphertext = is_tlwe<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_trlwe : std::false_type {};

template <torus_type Torus, uint32_t N>
struct is_trlwe<TRLWE<Torus, N>> : std::true_type {};

template <typename T>
concept trlwe_ciphertext = is_trlwe<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_trgsw : std::false_type {};

template <torus_type Torus, uint32_t N, uint32_t l>
struct is_trgsw<TRGSW<Torus, N, l>> : std::true_type {};

template <typename T>
concept trgsw_ciphertext = is_trgsw<std::remove_cvref_t<T>>::value;

template <typename params, typename... Feature>
class Cryptor {
 public:
  using Engine = std::mt19937;
  using params_type = params;
  using Torus = params::torus_type;

  Cryptor(std::shared_ptr<UInt::raw_value_type[]> s, Engine& eng)
      : secret_(std::move(s)), eng_(eng) {}

  template <typename Plaintext>
  auto encrypt(const Plaintext& pt) {
    if constexpr (tlwe_concept<params> && torus_type<Plaintext>) {
      return tlwe::encrypt<params>(secret_, eng_, pt);
    } else if constexpr (trlwe_concept<params> &&
                         torus_type<typename Plaintext::value_type>) {
      return trlwe::encrypt<params>(secret_, eng_, pt);
    } else if constexpr (trlwe_concept<params> &&
                         std::same_as<typename Plaintext::value_type, UInt>) {
      return trgsw::encrypt<params>(secret_, eng_, pt);
    } else {
      static_assert(always_false_v<Plaintext>, "Unsupported plaintext type");
    }
  }

  template <typename Ciphertext>
  auto decrypt(const Ciphertext& ct) {
    if constexpr (tlwe_concept<params> && tlwe_ciphertext<Ciphertext>) {
      return tlwe::decrypt<params>(secret_, ct);
    } else if constexpr (trlwe_concept<params> &&
                         (trlwe_ciphertext<Ciphertext> ||
                          tlwe_ciphertext<Ciphertext>)) {
      return trlwe::decrypt<params>(secret_, ct);
    } else {
      static_assert(always_false_v<Ciphertext>, "Unsupported ciphertext type");
    }
  }

 private:
  std::shared_ptr<UInt::raw_value_type[]> secret_;
  std::reference_wrapper<Engine> eng_;
};

#endif  // TFHE_CRYPTOR_HPP