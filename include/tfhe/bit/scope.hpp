// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SCOPE_HPP
#define TFHE_SCOPE_HPP

#include <cstdint>
#include <utility>

#include "tfhe/bit/bit.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/gate/hom_or.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"
#include "tfhe/utility/testvector.hpp"

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
        lhs.ready(), rhs.ready(), bk_));
  }

  Bit<Lwe, Rlwe> Or(const Bit<Lwe, Rlwe>& lhs,
                    const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomOr<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready(), rhs.ready(), bk_));
  }

  // lhs AND NOT rhs.
  Bit<Lwe, Rlwe> AndNot(const Bit<Lwe, Rlwe>& lhs,
                        const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready(), rhs.ready(), bk_));
  }

  Bit<Lwe, Rlwe> Xor(const Bit<Lwe, Rlwe>& lhs,
                     const Bit<Lwe, Rlwe>& rhs) const {
    return Bit<Lwe, Rlwe>(tfhe::gate::HomXor<Lwe, Rlwe, Decomp>::exec_impl(
        lhs.ready(), rhs.ready(), bk_));
  }

  // Bootstraps `bit` to fresh noise while moving its value from a
  // 1/InResolution step to a 1/OutResolution step -- e.g. a Bit lifted at
  // Dial<2, Torus> (0 or 1/2) that needs to become Dial<4, Torus> (0 or
  // 1/4) before feeding into And/Or/AndNot/Xor. InResolution == OutResolution
  // is a pure noise refresh with no value change.
  //
  // GateBootstrap's test vector (testvector::generate) always splits at
  // exactly 1/4 and 3/4 of the circle, classifying a message as the
  // negative half centered on 0 or the positive half centered on 1/2 -- it
  // has no separate threshold parameter. So rather than just offsetting
  // the input (which only shifts where 0/mu_in land, without changing how
  // far apart they are), this first scales the input up by
  // InResolution/2 -- via repeated self-addition, since no scalar-multiply
  // leveled op exists -- so its two possible values become exactly
  // {0, 1/2}: dead center of each half, for maximum noise margin. No
  // offset is then needed at all.
  template <uint32_t InResolution, uint32_t OutResolution>
  Bit<Lwe, Rlwe> Reslot(const Bit<Lwe, Rlwe>& bit) const {
    static_assert(InResolution % 2 == 0,
                  "Reslot needs InResolution/2 doublings of the input to "
                  "land its true value exactly on 1/2; an odd InResolution "
                  "can't reach that by repeated self-addition");
    constexpr uint32_t scale = InResolution / 2;
    static constexpr rTorus mu_out(1u, OutResolution);

    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu_out.value() >> 1u));

    TLWE<Torus, n> scaled = bit.ready();
    for (uint32_t i = 1; i < scale; ++i) {
      scaled = tfhe::leveled::Add<Lwe>::exec_impl(scaled, bit.ready());
    }

    return Bit<Lwe, Rlwe>(
        tfhe::bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>::exec_impl(
            mu_out, tv, scaled, bk_));
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
