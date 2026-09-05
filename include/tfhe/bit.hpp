// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BIT_HPP
#define TFHE_BIT_HPP

#include <cstdint>
#include <utility>
#include <variant>

#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/gate/hom_or.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/operation/leveled/key_switch.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Bit<Lwe, Rlwe> is a boolean ciphertext that hides whether it is
// currently Lwe-shaped (TLWE<Torus, n>, ready to feed straight into a
// gate) or Rlwe-shaped (TLWE<rTorus, N>, what every tfhe/gate/Hom* call
// returns) behind a single type, instead of a caller having to track that
// shape itself the way tfhe/circuit/binary_expansion.hpp's hand-written
// KeySwitch dance does. Only Lwe/Rlwe matter for that shape -- the
// bootstrap decomposition params a gate call additionally needs are the
// gate's own business, not this type's, so they aren't a template
// parameter here (see tfhe::bit::And and friends below).
//
// tfhe::bit::And/Or/AndNot/Xor materialize their
// operands automatically whenever they aren't already Lwe-shaped, so
// chaining gates never needs an explicit KeySwitch call to compile. They
// do this on local copies, though, and never mutate their arguments -- so
// materialize() is also public, letting a caller who wants control over
// exactly when that (comparatively expensive) step happens pay for it
// once up front instead, e.g. before reusing the same Bit across several
// gate calls, or skip it entirely for a value about to be decrypted
// directly under the Rlwe secret (see pending_ciphertext()).
template <typename Lwe, typename Rlwe>
class Bit {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  Bit(TLWE<Torus, n> ciphertext) : state_(std::move(ciphertext)) {}
  Bit(TLWE<rTorus, N> ciphertext) : state_(std::move(ciphertext)) {}

  // True once this is Lwe-shaped -- safe to read via ready_ciphertext()
  // without a materialize() first.
  bool is_ready() const {
    return std::holds_alternative<TLWE<Torus, n>>(state_);
  }

  // Valid only when is_ready().
  const TLWE<Torus, n>& ready_ciphertext() const {
    return std::get<TLWE<Torus, n>>(state_);
  }

  // Valid only when !is_ready(). Still decryptable directly under the
  // Rlwe secret's own coefficients (see Cryptor's sample-extracted
  // decrypt overload) -- a circuit's final output can read this straight
  // off without paying for a materialize() it doesn't need.
  const TLWE<rTorus, N>& pending_ciphertext() const {
    return std::get<TLWE<rTorus, N>>(state_);
  }

  // Converts in place to Lwe-shaped -- a no-op if already is_ready().
  // tfhe::bit::And/Or/AndNot/Xor call this on their own
  // local copies of their operands automatically; it's public so a
  // caller can instead choose to pay for it once, at a time of their
  // choosing.
  template <typename Kst>
  void materialize(const KeySwitchKey<Torus, n, Kst::t, N>& ksk) {
    if (is_ready()) return;
    state_ = tfhe::leveled::KeySwitch<ExtractedLwe<Rlwe>, Lwe, Kst>::exec_impl(
        pending_ciphertext(), ksk);
  }

 private:
  std::variant<TLWE<Torus, n>, TLWE<rTorus, N>> state_;
};

namespace tfhe::bit {

namespace detail {

// Shared by And/Or/AndNot/Xor below (see their own doc comment): a no-op
// for an operand already Lwe-shaped, and hands the result to whichever
// Hom* gate class the caller is instantiating for. Never mutates lhs/rhs
// themselves.
template <typename Kst, typename Gate, typename Lwe, typename Rlwe>
Bit<Lwe, Rlwe> combine(
    const Bit<Lwe, Rlwe>& lhs, const Bit<Lwe, Rlwe>& rhs,
    const BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Gate::l, Lwe::n>& bk,
    const KeySwitchKey<typename Lwe::torus_type, Lwe::n, Kst::t, Rlwe::N>&
        ksk) {
  Bit<Lwe, Rlwe> a = lhs;
  Bit<Lwe, Rlwe> b = rhs;
  a.template materialize<Kst>(ksk);
  b.template materialize<Kst>(ksk);
  return Bit<Lwe, Rlwe>(
      Gate::exec_impl(a.ready_ciphertext(), b.ready_ciphertext(), bk));
}

}  // namespace detail

template <typename Kst, typename Decomp, typename Lwe, typename Rlwe>
Bit<Lwe, Rlwe> And(const Bit<Lwe, Rlwe>& lhs, const Bit<Lwe, Rlwe>& rhs,
                   const BootstrapKey<typename Rlwe::torus_type, Rlwe::N,
                                      Decomp::l, Lwe::n>& bk,
                   const KeySwitchKey<typename Lwe::torus_type, Lwe::n, Kst::t,
                                      Rlwe::N>& ksk) {
  return detail::combine<Kst, gate::HomAnd<Lwe, Rlwe, Decomp>>(lhs, rhs, bk,
                                                               ksk);
}

template <typename Kst, typename Decomp, typename Lwe, typename Rlwe>
Bit<Lwe, Rlwe>
Or(const Bit<Lwe, Rlwe>& lhs, const Bit<Lwe, Rlwe>& rhs,
   const BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Decomp::l, Lwe::n>&
       bk,
   const KeySwitchKey<typename Lwe::torus_type, Lwe::n, Kst::t, Rlwe::N>& ksk) {
  return detail::combine<Kst, gate::HomOr<Lwe, Rlwe, Decomp>>(lhs, rhs, bk,
                                                              ksk);
}

// lhs AND NOT rhs.
template <typename Kst, typename Decomp, typename Lwe, typename Rlwe>
Bit<Lwe, Rlwe> AndNot(const Bit<Lwe, Rlwe>& lhs, const Bit<Lwe, Rlwe>& rhs,
                      const BootstrapKey<typename Rlwe::torus_type, Rlwe::N,
                                         Decomp::l, Lwe::n>& bk,
                      const KeySwitchKey<typename Lwe::torus_type, Lwe::n,
                                         Kst::t, Rlwe::N>& ksk) {
  return detail::combine<Kst, gate::HomAndNot<Lwe, Rlwe, Decomp>>(lhs, rhs, bk,
                                                                  ksk);
}

template <typename Kst, typename Decomp, typename Lwe, typename Rlwe>
Bit<Lwe, Rlwe> Xor(const Bit<Lwe, Rlwe>& lhs, const Bit<Lwe, Rlwe>& rhs,
                   const BootstrapKey<typename Rlwe::torus_type, Rlwe::N,
                                      Decomp::l, Lwe::n>& bk,
                   const KeySwitchKey<typename Lwe::torus_type, Lwe::n, Kst::t,
                                      Rlwe::N>& ksk) {
  return detail::combine<Kst, gate::HomXor<Lwe, Rlwe, Decomp>>(lhs, rhs, bk,
                                                               ksk);
}

}  // namespace tfhe::bit

#endif  // TFHE_BIT_HPP
