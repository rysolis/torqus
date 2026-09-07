// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_AND_HPP
#define TFHE_CIRCUIT_AND_HPP

#include "tfhe/bit/cipher.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

// Wraps tfhe::gate::HomAnd -- both operands must already be Lwe-shaped
// (Cipher::is_ready()); the result is Rlwe-shaped (not yet materialized),
// same as HomAnd's own output. Materialize it via Relay before feeding it
// into another call.
//
// Backend defaults to bootstrap::GateBootstrap -- see HomAnd's own doc
// comment for why this is a compile-time policy.
//
// Takes the BootstrapKey itself, not a BootstrapKeyHolder -- a caller
// holding one passes holder.bk() straight through; And has no need to
// know the holder concept exists. Holds a pointer to it, not a copy -- the
// referenced key must outlive this And.
namespace tfhe::circuit {

template <typename Lwe, typename Rlwe, typename Decomp,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap>
class And {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  And() = default;
  explicit And(const BootstrapKey<rTorus, N, l, n>& bk) : bk_(&bk) {}

  Cipher<Lwe, Rlwe> exec(const Cipher<Lwe, Rlwe>& lhs,
                         const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomAnd<Lwe, Rlwe, Decomp, Backend>::exec_impl(
            lhs.ready(), rhs.ready(), *bk_));
  }

 private:
  const BootstrapKey<rTorus, N, l, n>* bk_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_AND_HPP
