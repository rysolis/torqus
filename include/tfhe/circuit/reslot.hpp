// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_RESLOT_HPP
#define TFHE_CIRCUIT_RESLOT_HPP

#include "tfhe/bit/cipher.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/bootstrap/reslot.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

// Wraps tfhe::bootstrap::Reslot<InResolution, OutResolution> -- the operand
// must already be Lwe-shaped (Cipher::is_ready()); the result is
// Rlwe-shaped (not yet materialized), same shape tfhe::circuit::And/Or/
// AndNot/Xor's own results have. Materialize it via Relay before feeding
// it into another call.
//
// Backend defaults to bootstrap::GateBootstrap -- see HomAnd's own doc
// comment (tfhe/gate/hom_and.hpp) for why this is a compile-time policy.
//
// Takes the BootstrapKey itself, not a BootstrapKeyHolder -- a caller
// holding one passes holder.bk() straight through; Reslot has no need to
// know the holder concept exists. Holds a pointer to it, not a copy -- the
// referenced key must outlive this Reslot.
namespace tfhe::circuit {

template <uint32_t InResolution, uint32_t OutResolution, typename Lwe,
          typename Rlwe, typename Decomp,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap>
class Reslot {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  Reslot() = default;
  explicit Reslot(const BootstrapKey<rTorus, N, l, n>& bk) : bk_(&bk) {}

  Cipher<Lwe, Rlwe> exec(const Cipher<Lwe, Rlwe>& bit) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, Backend>::template exec_impl<
            InResolution, OutResolution>(bit.ready(), *bk_));
  }

 private:
  const BootstrapKey<rTorus, N, l, n>* bk_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_RESLOT_HPP
