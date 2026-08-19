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
#include "tfhe/operation/bootstrap/primitives/cmux.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

namespace tfhe::bootstrap {

template <typename Lwe, typename Rlwe, typename Decomp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Decomp>
class BlindRotate {
  static constexpr uint32_t n = Lwe::n;

  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Decomp::l;

 public:
  // NOTE:
  // exec_impl must not consume (move from) its arguments, as they are
  // forwarded again to tracking::update().
  static TRLWE<Torus, N> exec_impl(const TRLWE<Torus, N>& tv,
                                   const Vector<ModInt<M>, n + 1>& amount,
                                   const BootstrapKey<Torus, N, l, n>& bk) {
    ModInt<M> b = amount[n];
    TRLWE<Torus, N> cand0(rotate(tv.a(), -b.value()),
                          rotate(tv.b(), -b.value()));

    for (size_t i = 0; i < n; ++i) {
      ModInt<M> ai = amount[i];
      TRLWE<Torus, N> cand1(rotate(cand0.a(), ai.value()),
                            rotate(cand0.b(), ai.value()));

      cand0 = CMux<Rlwe, Decomp>::exec_impl(bk[i], cand0, cand1);
    }
    return cand0;
  }
};

}  // namespace tfhe::bootstrap

#endif  // TFHE_BLINDROTATE_HPP