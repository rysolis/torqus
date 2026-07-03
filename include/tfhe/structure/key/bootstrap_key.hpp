// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BOOTSTRAP_KEY_HPP
#define TFHE_BOOTSTRAP_KEY_HPP

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/trgsw.hpp"

template <torus_type Torus, uint32_t N, uint32_t l, uint32_t n>
class BootstrapKey {
 public:
  BootstrapKey() = default;
  TRGSW<Torus, N, l>& operator[](size_t idx) noexcept { return bks_[idx]; }
  const TRGSW<Torus, N, l>& operator[](size_t idx) const noexcept {
    return bks_[idx];
  }

 private:
  Vector<TRGSW<Torus, N, l>, n> bks_;
};

#endif  // TFHE_BOOTSTRAP_KEY_HPP