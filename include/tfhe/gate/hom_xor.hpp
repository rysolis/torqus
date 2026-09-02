// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_HOM_XOR_HPP
#define TFHE_HOM_XOR_HPP

#include <cstdint>

#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/utility/testvector.hpp"

// Combines c1/c2 (both Lwe-shaped) into their homomorphic XOR, the same
// Lwe-in/Rlwe-out shape HomAnd has -- see HomAnd. Unlike HomAnd/HomOr/
// HomAndNot, c1+c2 alone isn't linearly separable by GateBootstrap's fixed
// +-1/4 decision boundary (0/1/4/1/2 -- both same-bit cases land at 0 and
// 1/2, straddling the true region from opposite sides). Doubling the sum
// first collapses those same-bit cases onto the same point (0 and 1/2*2 =
// 1, i.e. 0 mod 1) while sending the differing-bit case to 1/2, right in
// the middle of the true region -- so no offset is needed, just the extra
// doubling.
namespace tfhe::gate {

template <typename Lwe, typename Rlwe, typename Decomp>
class HomXor {
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

    TLWE<Torus, n> sum = leveled::Add<Lwe>::exec_impl(c1, c2);
    TLWE<Torus, n> combined = leveled::Add<Lwe>::exec_impl(sum, sum);

    return bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>::exec_impl(mu, tv,
                                                                  combined, bk);
  }
};

}  // namespace tfhe::gate

#endif
