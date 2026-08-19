// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CMUX_HPP
#define TFHE_CMUX_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/poly.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation/bootstrap/primitives/external_product.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/operation/leveled/sub.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace tfhe::bootstrap {

template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
class CMux {
 public:
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t l = Decomp::l;

  template <torus_concept Torus>
  static TRLWE<Torus, N> exec_impl(const TRGSW<Torus, N, l>& bk,
                                   const TRLWE<Torus, N> cand0,
                                   const TRLWE<Torus, N> cand1) {
    return leveled::Add<Rlwe>::exec_impl(
        ExternalProduct<Rlwe, Decomp>::exec_impl(
            bk, leveled::Sub<Rlwe>::exec_impl(cand1, cand0)),
        cand0);
  }
};

}  // namespace tfhe::bootstrap

#endif  // TFHE_CMUX_HPP