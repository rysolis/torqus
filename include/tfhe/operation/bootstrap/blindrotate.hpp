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
  static TRLWE<Torus, N> exec_impl(const TRLWE<Torus, N>& tv,
                                   const Vector<ModInt<M>, n + 1>& amount,
                                   const BootstrapKey<Torus, N, l, n>& bk) {
    // rotate() takes its amount as uint32_t, which M (always a small
    // power of two, at most 2 * the ring dimension N) fits comfortably
    // regardless of ModInt<M>'s own Word -- (-b.value()) mod M is the
    // same value whether the negation happens in 32 or 64 bits, since M
    // divides both 2^32 and 2^64 evenly.
    ModInt<M> b = amount[n];
    uint32_t neg_b = static_cast<uint32_t>(-b.value());
    TRLWE<Torus, N> cand0(rotate(tv.a(), neg_b), rotate(tv.b(), neg_b));

    for (size_t i = 0; i < n; ++i) {
      ModInt<M> ai = amount[i];
      uint32_t ai_amount = static_cast<uint32_t>(ai.value());
      TRLWE<Torus, N> cand1(rotate(cand0.a(), ai_amount),
                            rotate(cand0.b(), ai_amount));

      cand0 = CMux<Rlwe, Decomp>::exec_impl(bk[i], cand0, cand1);
    }
    return cand0;
  }
};

}  // namespace tfhe::bootstrap

#endif  // TFHE_BLINDROTATE_HPP