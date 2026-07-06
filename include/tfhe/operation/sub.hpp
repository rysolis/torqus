// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SUB_HPP
#define TFHE_SUB_HPP

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <trlwe_concept params>
class Sub {
 public:
  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  inline static TRLWE<Torus, N> exec(TRLWE<Torus, N> lhs,
                                     const TRLWE<Torus, N>& rhs) {
    return lhs -= rhs;
  }
};

#endif