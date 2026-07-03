// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BLINDROTATE_HPP
#define TFHE_BLINDROTATE_HPP

#include <cmath>

#include "primitive/modint.hpp"

#include "algebra/rotate.hpp"
#include "algebra/vector.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/math/modswitch.hpp"
#include "tfhe/operation/cmux.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

template <tlwe_concept fparams, trlwe_concept bparams>
  requires decompose_concept<bparams>
class BlindRotate {
  using bTorus = typename bparams::torus_type;

  static constexpr uint32_t n = fparams::n;
  static constexpr uint32_t N = bparams::N;
  static constexpr uint32_t l = bparams::l;
  static constexpr uint32_t M = 2 * N;

 public:
  TRLWE<bTorus, N> operator()(TRLWE<bTorus, N>& tv,
                              const Vector<ModInt<M>, n + 1>& t,
                              const BootstrapKey<bTorus, N, l, n>& bk) {
    ModInt<M> b = t[n];
    TRLWE<bTorus, N> cand0(rotate(tv.a(), -b.value()),
                           rotate(tv.b(), -b.value()));

    for (size_t i = 0; i < n; ++i) {
      ModInt<M> ai = t[i];
      TRLWE<bTorus, N> cand1(rotate(cand0.a(), ai.value()),
                             rotate(cand0.b(), ai.value()));

      cand0 = cmux_(bk[i], cand0, cand1);
    }
    return cand0;
  }

  CMux<bparams> cmux_;
};

#endif  // TFHE_BLINDROTATE_HPP