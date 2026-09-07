// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_HPP
#define TFHE_CIRCUIT_HPP

#include <cstdint>
#include <utility>

#include "tfhe/bit/cipher.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/gate/hom_or.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/bootstrap/reslot.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Circuit<Lwe, Rlwe, Decomp> holds the BootstrapKey a gate call needs,
// exposing And/Or/AndNot/Xor as methods instead of a call site spelling
// out <Kst, Decomp> and bk by hand. Both operands must already be
// Lwe-shaped (Cipher::is_ready()) -- materialize a gate's own output via
// Relay::materialize() before feeding it into another call.
//
// Backend defaults to bootstrap::GateBootstrap and is forwarded to each
// tfhe::gate::Hom* call and to Reslot's own bootstrap -- see HomAnd's own
// doc comment for why this is a compile-time policy, not a runtime
// parameter. Circuit<Lwe,Rlwe,Decomp> (Backend omitted) is byte-for-byte
// today's Circuit; a caller that wants a non-default Backend just spells
// it out, e.g. Circuit<Lwe,Rlwe,Decomp,MyHardwareBootstrap>.
template <typename Lwe, typename Rlwe, typename Decomp,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap>
class Circuit {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  Circuit() = default;
  explicit Circuit(BootstrapKey<rTorus, N, l, n> bk) : bk_(std::move(bk)) {}

  Cipher<Lwe, Rlwe> And(const Cipher<Lwe, Rlwe>& lhs,
                        const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomAnd<Lwe, Rlwe, Decomp, Backend>::exec_impl(
            lhs.ready(), rhs.ready(), bk_));
  }

  Cipher<Lwe, Rlwe> Or(const Cipher<Lwe, Rlwe>& lhs,
                       const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomOr<Lwe, Rlwe, Decomp, Backend>::exec_impl(
            lhs.ready(), rhs.ready(), bk_));
  }

  // lhs AND NOT rhs.
  Cipher<Lwe, Rlwe> AndNot(const Cipher<Lwe, Rlwe>& lhs,
                           const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp, Backend>::exec_impl(
            lhs.ready(), rhs.ready(), bk_));
  }

  Cipher<Lwe, Rlwe> Xor(const Cipher<Lwe, Rlwe>& lhs,
                        const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomXor<Lwe, Rlwe, Decomp, Backend>::exec_impl(
            lhs.ready(), rhs.ready(), bk_));
  }

  // Bootstraps `bit` to fresh noise while moving its value from a
  // 1/InResolution step to a 1/OutResolution step -- e.g. a Cipher lifted at
  // Dial<2, Torus> (0 or 1/2) that needs to become Dial<4, Torus> (0 or
  // 1/4) before feeding into And/Or/AndNot/Xor. InResolution == OutResolution
  // is a pure noise refresh with no value change. The result isn't
  // materialized -- call Relay::materialize() before feeding it into
  // another call, same as And/Or/AndNot/Xor's own results.
  //
  // The actual algorithm (see tfhe::bootstrap::Reslot's own doc comment)
  // takes/returns raw TLWE, not Cipher -- this is a thin convenience wrapper
  // around it, matching how And/Or/AndNot/Xor wrap tfhe::gate::HomAnd/
  // HomOr/HomAndNot/HomXor.
  template <uint32_t InResolution, uint32_t OutResolution>
  Cipher<Lwe, Rlwe> Reslot(const Cipher<Lwe, Rlwe>& bit) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::bootstrap::Reslot<Lwe, Rlwe, Decomp, Backend>::template exec_impl<
            InResolution, OutResolution>(bit.ready(), bk_));
  }

 private:
  BootstrapKey<rTorus, N, l, n> bk_;
};

// Relay<Lwe, Rlwe, Kst> holds the KeySwitchKey needed to materialize a
// Cipher -- converting a gate's Rlwe-shaped result back down to Lwe-shaped so
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
  void materialize(Cipher<Lwe, Rlwe>& bit) const {
    bit.template materialize<Kst>(ksk_);
  }

 private:
  KeySwitchKey<Torus, n, t, N> ksk_;
};

#endif  // TFHE_CIRCUIT_HPP
