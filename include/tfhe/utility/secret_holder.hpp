// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_SECRET_HOLDER_HPP
#define TFHE_UTILITY_SECRET_HOLDER_HPP

#include <cstdint>
#include <memory>
#include <random>

#include "primitive/uint.hpp"

#include "algebra/poly.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/params.hpp"

template <typename params>
struct secret_size;

template <tlwe_concept params>
struct secret_size<params> {
  static constexpr uint32_t value = params::n;
};

template <typename params>
  requires trlwe_concept<params>
struct secret_size<params> {
  static constexpr uint32_t value = params::N;
};

template <typename params>
class SecretHolder {
 public:
  using raw_value_type = UInt::raw_value_type;
  using iterator = raw_value_type*;
  using const_iterator = const raw_value_type*;

  static constexpr uint32_t size = secret_size<params>::value;

  template <typename Engine>
  explicit SecretHolder(Engine& eng) {
    std::uniform_int_distribution<raw_value_type> dist{0, 1};

    secret_ = std::make_shared<raw_value_type[]>(size);
    std::generate(secret_.get(), secret_.get() + size,
                  [&] { return dist(eng); });
  }

  template <std::forward_iterator It>
  SecretHolder(It first, It last) {
    assert(std::distance(first, last) == size);
    secret_ = std::make_shared<raw_value_type[]>(size);
    std::copy(first, last, secret_.get());
  }

  const raw_value_type* secret() const noexcept { return secret_.get(); }
  std::shared_ptr<raw_value_type[]> secret_ptr() const noexcept {
    return secret_;
  }

  iterator begin() noexcept { return secret_.get(); }
  iterator end() noexcept { return secret_.get() + size; }

  const_iterator begin() const noexcept { return secret_.get(); }
  const_iterator end() const noexcept { return secret_.get() + size; }

 protected:
  std::shared_ptr<raw_value_type[]> secret_;
};

#endif  // TFHE_KEYRING_HPP