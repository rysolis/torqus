// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_RESLOT_HPP
#define TFHE_CIRCUIT_RESLOT_HPP

#include "tfhe/cipher/cipher.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/bootstrap/reslot.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Wraps tfhe::bootstrap::Reslot<InResolution, OutResolution> -- the operand
// must already be Lwe-shaped (Cipher::is_ready()). exec() returns the
// Rlwe-shaped (not yet materialized) result, same shape any Bootstrap gate
// result has. materialize() converts it to a ready-to-use, Lwe-shaped
// result -- a no-op if the Cipher already is one.
//
// Backend defaults to bootstrap::GateBootstrap -- see HomAnd's own doc
// comment (tfhe/gate/hom_and.hpp) for why this is a compile-time policy.
// FuseKeySwitch (false by default) picks which of Backend's two
// exec_impl overloads exec() calls -- true calls its Kst-templated one,
// making exec() return an already Lwe-shaped result (materialize() stays
// a no-op); false calls the plain one as usual. An explicit choice, not
// detected from Backend -- Backend must actually provide the
// Kst-templated overload when FuseKeySwitch is true.
//
// Takes the BootstrapKey/KeySwitchKey directly -- a caller just passes its
// own key values straight through. Holds raw pointers to both (not
// copies) -- the referenced keys must outlive this Reslot.
namespace tfhe::circuit {

template <uint32_t InResolution, uint32_t OutResolution, typename Lwe,
          typename Rlwe, typename Decomp, typename Kst,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap,
          bool FuseKeySwitch = false>
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
      : bk_(&bk), ksk_(&ksk) {}

  Cipher<Lwe, Rlwe> exec(const Cipher<Lwe, Rlwe>& bit) const {
    if constexpr (FuseKeySwitch) {
      return Cipher<Lwe, Rlwe>(
          tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, Backend>::
              template exec_impl<InResolution, OutResolution, Kst>(
                  bit.ready(), *bk_, *ksk_));
    } else {
      return Cipher<Lwe, Rlwe>(
          tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, Backend>::
              template exec_impl<InResolution, OutResolution>(bit.ready(),
                                                              *bk_));
    }
  }

  // Converts bit in place to Lwe-shaped via KeySwitch -- a no-op if
  // already is_ready().
  const TLWE<Torus, n>& materialize(Cipher<Lwe, Rlwe>& bit) const {
    bit.template materialize<Kst>(*ksk_);
    return bit.ready();
  }

 private:
  const BootstrapKey<rTorus, N, l, n>* bk_;
  const KeySwitchKey<Torus, n, t, N>* ksk_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_RESLOT_HPP
