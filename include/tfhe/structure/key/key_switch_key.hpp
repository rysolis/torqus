// Copyright 2026 EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_KEY_SWITCH_KEY_HPP
#define TFHE_KEY_SWITCH_KEY_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/tlwe.hpp"

template <torus_type Torus, uint32_t n, uint32_t t, uint32_t N>
class KeySwitchKey {
 public:
  KeySwitchKey() = default;
  Vector<TLWE<Torus, n>, t>& operator[](size_t idx) noexcept {
    return ks_[idx];
  }
  const Vector<TLWE<Torus, n>, t>& operator[](size_t idx) const noexcept {
    return ks_[idx];
  }

 private:
  Vector<Vector<TLWE<Torus, n>, t>, N> ks_;
};

#endif
