// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_GATE_BOOTSTRAP_HPP
#define TFHE_GATE_BOOTSTRAP_HPP

#include <cstdint>

#include "tfhe/math/modswitch.hpp"
#include "tfhe/operation/add.hpp"
#include "tfhe/operation/blindrotate.hpp"
#include "tfhe/operation/sample_extraction.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

template <typename Lwe, typename Rlwe, typename Decomp>
class GateBootstrap {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Decomp::l;

  // NOTE:
  // exec_impl must not consume (move from) its arguments, as they are
  // forwarded again to tracking::update().
  static TLWE<rTorus, N> exec_impl(const rTorus mu, const TRLWE<rTorus, N>& tv,
                                   const TLWE<Torus, n>& tlwe,
                                   const BootstrapKey<rTorus, N, l, n>& bk) {
    // convert TLWE to Vector<ModInt<M>>
    Vector<ModInt<M>, n + 1> amount;
    constexpr uint32_t Q = [] {
      if constexpr (Torus::qbit == 32) {
        return 0;
      } else {
        return 1 << Torus::qbit;
      }
    }();
    for (size_t i = 0; i < n; ++i) {
      amount[i] = mod_switch<M>(ModInt<Q>(tlwe.a()[i].value()));
    }
    amount[n] = mod_switch<M>(ModInt<Q>(tlwe.b().value()));

    // BlindRotate
    TRLWE<rTorus, N> rot =
        BlindRotate<Lwe, Rlwe, Decomp>::exec_impl(tv, amount, bk);

    // prepare offset
    TLWE<rTorus, N> offset;
    offset.b() = rTorus(mu.value() / 2);

    return Add<Rlwe>::exec_impl(offset,
                                SampleExtraction<Lwe, Rlwe>::exec_impl(rot, 0));
  }
};

#endif
