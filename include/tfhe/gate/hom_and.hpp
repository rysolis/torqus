// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_HOM_AND_HPP
#define TFHE_HOM_AND_HPP

#include <cstdint>

#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/utility/testvector.hpp"

// Combines c1/c2 (both Lwe-shaped) into their homomorphic AND, returning
// the result as a fresh Rlwe-domain ciphertext via Backend -- the same
// Lwe-in/Rlwe-out shape GateBootstrap itself has. Chaining several of
// these together (as BinaryExpansion does) therefore needs a KeySwitch
// back down to Lwe between calls; that's the caller's job (see
// BinaryExpansion), not this one's.
//
// Backend defaults to bootstrap::GateBootstrap (today's software
// implementation) -- a compile-time policy, not a runtime parameter, since
// which one a given Circuit should use is a build-time choice (see
// Circuit, which is what actually threads a non-default Backend down to
// here); a program can still hold software- and hardware-backed Circuit
// instances side by side, they're just different types.
namespace tfhe::gate {

template <typename Lwe, typename Rlwe, typename Decomp,
          template <typename, typename, typename> class Backend =
              bootstrap::GateBootstrap>
class HomAnd {
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
    offset.b() = -Torus(1u, 8u);

    TLWE<Torus, n> combined = leveled::Add<Lwe>::exec_impl(
        offset, leveled::Add<Lwe>::exec_impl(c1, c2));

    return Backend<Lwe, Rlwe, Decomp>::exec_impl(mu, tv, combined, bk);
  }
};

}  // namespace tfhe::gate

#endif
