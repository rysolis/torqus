// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CMUX_HPP
#define TFHE_CMUX_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/poly.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation/add.hpp"
#include "tfhe/operation/external_product.hpp"
#include "tfhe/operation/sub.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <decompose_concept params>
class CMux {
 public:
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  template <torus_type Torus>
  inline static TRLWE<Torus, params::N> exec(
      const TRGSW<Torus, params::N, params::l>& bk,
      const TRLWE<Torus, params::N> cand0,
      const TRLWE<Torus, params::N> cand1) {
    return Add<params>::exec(
        ExternalProduct<params>::exec(bk, Sub<params>::exec(cand1, cand0)),
        cand0);
  }
};

#endif  // TFHE_CMUX_HPP