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

  static TRLWE<Torus, N> exec_impl(const TRLWE<Torus, N>& lhs,
                                   const TRLWE<Torus, N>& rhs) {
    TRLWE<Torus, N> result(lhs);
    return result -= rhs;
  }
};

template <tlwe_concept Params>
class Sub<Params> {
 public:
  using Torus = typename Params::torus_type;
  static constexpr uint32_t n = Params::n;

  static TLWE<Torus, n> exec_impl(const TLWE<Torus, n>& lhs,
                                  const TLWE<Torus, n>& rhs) {
    TLWE<Torus, n> result(lhs);
    return result -= rhs;
  }
};

}  // namespace tfhe::leveled

#endif