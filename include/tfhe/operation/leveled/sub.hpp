// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SUB_HPP
#define TFHE_SUB_HPP

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace tfhe::leveled {

template <typename Params>
class Sub;

template <trlwe_concept Params>
class Sub<Params> {
 public:
  using Torus = typename Params::torus_type;
  static constexpr uint32_t N = Params::N;

  // NOTE:
  // exec_impl must not consume (move from) its arguments, as they are
  // forwarded again to tracking::update().
  static TRLWE<Torus, N> exec_impl(TRLWE<Torus, N> lhs,
                                   const TRLWE<Torus, N>& rhs) {
    return lhs -= rhs;
  }
};

template <tlwe_concept Params>
class Sub<Params> {
 public:
  using Torus = typename Params::torus_type;
  static constexpr uint32_t n = Params::n;

  static TLWE<Torus, n> exec_impl(TLWE<Torus, n> lhs,
                                  const TLWE<Torus, n>& rhs) {
    return lhs -= rhs;
  }
};

}  // namespace tfhe::leveled

#endif