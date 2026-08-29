// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_HOM_OR_HPP
#define TFHE_HOM_OR_HPP

#include <cstdint>

#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/utility/testvector.hpp"

// Combines c1/c2 (both Lwe-shaped) into their homomorphic OR, the same
// Lwe-in/Rlwe-out shape HomAnd has -- see HomAnd. Only the offset's sign
// differs from HomAnd's (+1/8 here vs. -1/8 there), shifting the same
// decision boundary so that either input being true is enough.
namespace tfhe::gate {

template <typename Lwe, typename Rlwe, typename Decomp>
class HomOr {
 public:
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  static constexpr uint32_t l = Decomp::l;

  static TLWE<rTorus, N> exec_impl(const TLWE<Torus, n>& c1,
                                   const TLWE<Torus, n>& c2,
                                   const BootstrapKey<rTorus, N, l, n>& bk) {
    static constexpr Torus mu(1u, 4u);
    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu.value() >> 1u));

    TLWE<Torus, n> offset;
    offset.b() = Torus(1u, 8u);

    TLWE<Torus, n> combined = leveled::Add<Lwe>::exec_impl(
        offset, leveled::Add<Lwe>::exec_impl(c1, c2));

    return bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>::exec_impl(mu, tv,
                                                                  combined, bk);
  }
};

}  // namespace tfhe::gate

#endif
