// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_ADD_HPP
#define TFHE_ADD_HPP

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <trlwe_concept params>
class Add {
 public:
  using Torus = typename params::torus_type;
  static constexpr uint32_t N = params::N;

  // NOTE:
  // exec_impl must not consume (move from) its arguments, as they are
  // forwarded again to tracking::update().
  static TRLWE<Torus, N> exec_impl(TRLWE<Torus, N> lhs,
                                   const TRLWE<Torus, N>& rhs) {
    return lhs += rhs;
  }

  static TLWE<Torus, N> exec_impl(TLWE<Torus, N> lhs,
                                  const TLWE<Torus, N>& rhs) {
    return lhs += rhs;
  }
};

#endif