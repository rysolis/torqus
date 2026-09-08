// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_RESLOT_HPP
#define TFHE_CIRCUIT_RESLOT_HPP

#include <utility>

#include "tfhe/bit/cipher.hpp"
#include "tfhe/circuit/circuit.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/bootstrap/reslot.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

// Wraps tfhe::bootstrap::Reslot<InResolution, OutResolution> -- the operand
// must already be Lwe-shaped (Cipher::is_ready()). exec() returns the
// Rlwe-shaped (not yet materialized) result, same shape any Bootstrap gate
// result has -- for a caller chaining more gate calls before materializing.
// exec_ready() does the same bootstrap plus a Relay::materialize() in one
// call, for a caller that just wants a ready-to-use, Lwe-shaped result (see
// BinaryExpansion::exec_ready's own doc comment -- same idea, single value
// instead of an array of slots).
//
// Backend defaults to bootstrap::GateBootstrap -- see HomAnd's own doc
// comment (tfhe/gate/hom_and.hpp) for why this is a compile-time policy.
//
// Takes the BootstrapKey/KeySwitchKey directly, not a Holder -- a caller
// holding one passes holder.bk()/holder.ksk() straight through; Reslot has
// no need to know the holder concept exists. Holds a pointer to the
// BootstrapKey (not a copy) and its own Relay built from the KeySwitchKey
// -- the referenced keys must outlive this Reslot.
namespace tfhe::circuit {

template <uint32_t InResolution, uint32_t OutResolution, typename Lwe,
          typename Rlwe, typename Decomp, typename Kst,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap>
class Reslot {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  Reslot() = default;
  Reslot(const BootstrapKey<rTorus, N, l, n>& bk,
         const KeySwitchKey<Torus, n, t, N>& ksk)
      : bk_(&bk), relay_(ksk) {}

  Cipher<Lwe, Rlwe> exec(const Cipher<Lwe, Rlwe>& bit) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, Backend>::template exec_impl<
            InResolution, OutResolution>(bit.ready(), *bk_));
  }

  TLWE<Torus, n> exec_ready(const Cipher<Lwe, Rlwe>& bit) const {
    Cipher<Lwe, Rlwe> resloted = exec(bit);
    relay_.materialize(resloted);
    return std::move(resloted).ready();
  }

 private:
  const BootstrapKey<rTorus, N, l, n>* bk_;
  Relay<Lwe, Rlwe, Kst> relay_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_RESLOT_HPP
