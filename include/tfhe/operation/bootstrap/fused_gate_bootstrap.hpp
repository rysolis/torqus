// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_FUSED_GATE_BOOTSTRAP_HPP
#define TFHE_FUSED_GATE_BOOTSTRAP_HPP

#include <concepts>
#include <cstdint>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/leveled/key_switch.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

namespace tfhe::bootstrap {

// Whether Backend<Lwe, Rlwe, Decomp, Kst> implements the Bootstrap+
// KeySwitch granularity below (KeySwitchKey alongside BootstrapKey,
// already-KeySwitched Lwe-dimension result) rather than GateBootstrap's
// Bootstrap-only one. Checked structurally, not nominally.
template <template <typename...> class Backend, typename Lwe, typename Rlwe,
          typename Decomp, typename Kst>
concept fused_gate_backend_concept = requires(
    const typename Rlwe::torus_type mu,
    const TRLWE<typename Rlwe::torus_type, Rlwe::N>& tv,
    const TLWE<typename Lwe::torus_type, Lwe::n>& tlwe,
    const BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Decomp::l, Lwe::n>&
        bk,
    const KeySwitchKey<typename Lwe::torus_type, Lwe::n, Kst::t, Rlwe::N>&
        ksk) {
  {
    Backend<Lwe, Rlwe, Decomp, Kst>::exec_impl(mu, tv, tlwe, bk, ksk)
  } -> std::same_as<TLWE<typename Lwe::torus_type, Lwe::n>>;
};

// Software reference for that granularity: GateBootstrap, then an
// immediate KeySwitch. Not used by default.
template <typename Lwe, typename Rlwe, typename Decomp, typename Kst>
  requires kst_concept<Kst>
class FusedGateBootstrap {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  static TLWE<Torus, n> exec_impl(const rTorus mu, const TRLWE<rTorus, N>& tv,
                                  const TLWE<Torus, n>& tlwe,
                                  const BootstrapKey<rTorus, N, l, n>& bk,
                                  const KeySwitchKey<Torus, n, t, N>& ksk) {
    return leveled::KeySwitch<ExtractedLwe<Rlwe>, Lwe, Kst>::exec_impl(
        GateBootstrap<Lwe, Rlwe, Decomp>::exec_impl(mu, tv, tlwe, bk), ksk);
  }
};

}  // namespace tfhe::bootstrap

#endif
