// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SCOPE_HPP
#define TFHE_SCOPE_HPP

#include <cstdint>
#include <utility>

#include "tfhe/bit.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/gate/hom_or.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Circuit<Lwe, Rlwe, Decomp> holds the BootstrapKey a gate call needs,
// exposing And/Or/AndNot/Xor as methods instead of a call site spelling
// out <Kst, Decomp> and bk by hand. Both operands must already be
// Lwe-shaped (Bit::is_ready()) -- materialize a gate's own output via
// Relay::materialize() before feeding it into another call.
template <typename Lwe, typename Rlwe, typename Decomp>
class Circuit {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  Circuit() = default;
  explicit Circuit(BootstrapKey<rTorus, N, l, n> bk) : bk_(std::move(bk)) {}

  Bit<Lwe, Rlwe> And(const Bit<Lwe, Rlwe>& lhs,
                     const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomAnd<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready_ciphertext(), rhs.ready_ciphertext(), bk_));
  }

  Bit<Lwe, Rlwe> Or(const Bit<Lwe, Rlwe>& lhs,
                    const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomOr<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready_ciphertext(), rhs.ready_ciphertext(), bk_));
  }

  // lhs AND NOT rhs.
  Bit<Lwe, Rlwe> AndNot(const Bit<Lwe, Rlwe>& lhs,
                        const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready_ciphertext(), rhs.ready_ciphertext(), bk_));
  }

  Bit<Lwe, Rlwe> Xor(const Bit<Lwe, Rlwe>& lhs,
                     const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomXor<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready_ciphertext(), rhs.ready_ciphertext(), bk_));
  }

 private:
  BootstrapKey<rTorus, N, l, n> bk_;
};

// Relay<Lwe, Rlwe, Kst> holds the KeySwitchKey needed to materialize a
// Bit -- converting a gate's Rlwe-shaped result back down to Lwe-shaped so
// it can feed into another Circuit call.
template <typename Lwe, typename Rlwe, typename Kst>
class Relay {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t t = Kst::t;

  Relay() = default;
  explicit Relay(KeySwitchKey<Torus, n, t, N> ksk) : ksk_(std::move(ksk)) {}

  // Converts `bit` in place to Lwe-shaped -- a no-op if already
  // bit.is_ready().
  void materialize(Bit<Lwe, Rlwe>& bit) const {
    bit.template materialize<Kst>(ksk_);
  }

 private:
  KeySwitchKey<Torus, n, t, N> ksk_;
};

#endif  // TFHE_SCOPE_HPP
