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

template <uint32_t SecretSize>
class SecretHolder {
 public:
  using raw_value_type = UInt::raw_value_type;

  template <typename Engine>
    requires std::uniform_random_bit_generator<Engine>
  explicit SecretHolder(Engine& eng) {
    std::uniform_int_distribution<raw_value_type> dist{0, 1};
    std::generate(secret_.get(), secret_.get() + SecretSize,
                  [&] { return dist(eng); });
  }

  const raw_value_type* get() const noexcept { return secret_.get(); }
  std::shared_ptr<raw_value_type[]> shared_get() const noexcept {
    return secret_;
  }

  static constexpr uint32_t size() { return SecretSize; }

 private:
  std::shared_ptr<raw_value_type[]> secret_ =
      std::make_shared<raw_value_type[]>(SecretSize);
};

#endif  // TFHE_UTILITY_SECRET_HOLDER_HPP