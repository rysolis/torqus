// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_NOISE_HPP
#define TFHE_NOISE_HPP

#include <cmath>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation.hpp"
#include "tfhe/utility/analysis/exact_bound.hpp"

template <typename Op>
struct NoisePolicy;

// Fact 3.5 (linear combination of valid TLWE/TRLWE samples), coefficients
// e1=e2=1: ||Err(c1+c2)||_inf <= ||e1||_1*||Err(c1)||_inf +
// ||e2||_1*||Err(c2)||_inf = ||Err(c1)||_inf + ||Err(c2)||_inf.
template <trlwe_concept Params>
struct NoisePolicy<tfhe::leveled::Add<Params>> {
  static constexpr uint32_t N = Params::N;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return to_round_up(to_exact(tracker->get(lhs)) +
                       to_exact(tracker->get(rhs)));
  }
};

// Same as NoisePolicy<Add> above (Fact 3.5): |e1-e2| <= |e1|+|e2|, so
// subtraction's worst-case bound is the same sum, not a difference.
template <trlwe_concept Params>
struct NoisePolicy<tfhe::leveled::Sub<Params>> {
  static constexpr uint32_t N = Params::N;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return to_round_up(to_exact(tracker->get(lhs)) +
                       to_exact(tracker->get(rhs)));
  }
};

// Theorem 3.13 (Worst-case External Product, Chillotti et al., "TFHE: Fast
// Fully Homomorphic Encryption over the Torus", eprint.iacr.org/2018/421),
// specialized to this codebase's k=1 (single-mask TRLWE/TRGSW):
//   ||Err(A box b)||_inf <= (k+1)*l*N*beta*||Err(A)||_inf
//                           + ||muA||_1*(1+kN)*eps + ||muA||_1*||Err(b)||_inf
// with beta=B/2 (Lemma 3.7's gadget quality) and ||muA||_1 <= 1 for a
// bit-message TRGSW A (here `lhs`).
template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct NoisePolicy<tfhe::bootstrap::ExternalProduct<Rlwe, Decomp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static ExactBound eps() {
    return ExactBound(1, boost::multiprecision::cpp_int(1) << (Bbit * l + 1));
  }

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    ExactBound bound =
        (ExactBound(2 * l * N * (B >> 1)) * to_exact(tracker->get(lhs))) +
        (ExactBound(1 + N) * eps()) + to_exact(tracker->get(rhs));
    return to_round_up(bound);
  }
};

// Lemma 3.16 (CMux gate), k=1:
//   ||Err(CMux(C,d1,d0))||_inf <= max(||Err(d0)||_inf,||Err(d1)||_inf) + eta(C)
//   eta(C) = (k+1)*l*N*beta*||Err(C)||_inf + (kN+1)*eps
template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct NoisePolicy<tfhe::bootstrap::CMux<Rlwe, Decomp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static ExactBound eps() {
    return ExactBound(1, boost::multiprecision::cpp_int(1) << (Bbit * l + 1));
  }

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker,
                        const TRGSW<Torus, N, l>& selector,
                        const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    ExactBound max_input =
        std::max(to_exact(tracker->get(lhs)), to_exact(tracker->get(rhs)));
    ExactBound bound =
        (ExactBound(2 * l * N * (B >> 1)) * to_exact(tracker->get(selector))) +
        (ExactBound(1 + N) * eps()) + max_input;
    return to_round_up(bound);
  }
};

// Theorem 4.3 (BlindRotate), k=1, p=n CMux rounds:
//   ||Err(ACC)||_inf <= ||Err(c)||_inf + p*(k+1)*l*N*beta*A_C + p*(1+kN)*eps
// The ||Err(c)||_inf term is dropped here (see "assume ||Err(tv)||=0"
// below) -- not a shortcut, but exactly how the paper itself uses this
// theorem for gate bootstrapping (Theorem 6.2/Algorithm 9): the
// accumulator's starting test vector v is a noiseless trivial plaintext
// polynomial, never an encrypted ciphertext.
template <typename Lwe, typename Rlwe, typename Decomp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct NoisePolicy<tfhe::bootstrap::BlindRotate<Lwe, Rlwe, Decomp>> {
  static constexpr uint32_t n = Lwe::n;
  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static ExactBound eps() {
    return ExactBound(1, boost::multiprecision::cpp_int(1) << (Bbit * l + 1));
  }

  template <typename Tracker>
  static double compute(const Tracker*, const TRLWE<Torus, N>&,
                        const Vector<ModInt<M>, n + 1>&,
                        const BootstrapKey<Torus, N, l, n>& bk) {
    // assume that ||Err(tv)|| = 0
    ExactBound bound = (ExactBound(n * 2 * l * N * (B >> 1)) *
                        to_exact(get_key_noise_tracker_if()->get(bk))) +
                       (ExactBound(n * (1 + N)) * eps());
    return to_round_up(bound);
  }
};
// Same formula as NoisePolicy<BlindRotate> above (Theorem 4.3): this op's
// own noise contribution IS BlindRotate's, since the trailing
// SampleExtract/Add(offset) steps are noise-free. Corresponds to the
// paper's Theorem 6.2 "Bootstrapping TLWE-to-TLWE" (Algorithm 9) -- note
// the paper's own "Gate Bootstrapping" (Theorem 6.3/Algorithm 10) instead
// bundles a KeySwitch right after this, whereas here that composition
// happens at the call site instead (e.g. a downstream consumer composing
// E(seal) = E(GateBootstrap) + E(KeySwitch)), so this Op's own analysis
// stays Theorem 6.2's, without Theorem 6.3's extra key-switch terms.
template <typename Lwe, typename Rlwe, typename Decomp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct NoisePolicy<tfhe::bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>> {
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static ExactBound eps() {
    return ExactBound(1, boost::multiprecision::cpp_int(1) << (Bbit * l + 1));
  }

  template <typename Tracker>
  static double compute(const Tracker*, const rTorus, const TRLWE<rTorus, N>&,
                        const TLWE<Torus, n>&,
                        const BootstrapKey<rTorus, N, l, n>& bk) {
    ExactBound bound = (ExactBound(n * 2 * l * N * (B >> 1)) *
                        to_exact(get_key_noise_tracker_if()->get(bk))) +
                       (ExactBound(n * (1 + N)) * eps());
    return to_round_up(bound);
  }
};

// NOTE on provenance:
// Theorem 4.1 (Chillotti et al., "TFHE: Fast Fully Homomorphic Encryption
// over the Torus", eprint.iacr.org/2018/421) proves
//   ‖Err(c)‖∞ ≤ ‖Err(src)‖∞ + n·t·N·A_KS + n·2^-(t+1)
// but only for BINARY decomposition (Algorithm 2 decomposes into bits
// ãi,j ∈ {0,1}, i.e. it implicitly assumes digit magnitude ≤ 1 = K-1 for
// K=2). This codebase generalizes KeySwitchKey to an arbitrary base K
// (Kst::K), which the paper does not cover. The (K-1) factor below and the
// Kbit-based `eps` are our own extension, obtained by applying the same
// digit-magnitude/count bookkeeping that Theorem 3.13 uses for a general
// gadget base Bg (via Lemma 3.7's β = Bg/2, ε = 1/(2·Bg^ℓ)) to Theorem 4.1's
// proof structure instead. They reduce exactly to the paper's formula when
// K=2 (Kbit=1, K-1=1). See docs/noise_bound_proof.md for the derivation.
template <typename Src, typename Dst, typename Kst>
  requires tlwe_concept<Src> && tlwe_concept<Dst> && kst_concept<Kst>
struct NoisePolicy<tfhe::leveled::KeySwitch<Src, Dst, Kst>> {
  static constexpr uint32_t n = Dst::n;
  using Torus = typename Src::torus_type;
  static constexpr uint32_t N = Src::n;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  static constexpr uint32_t Kbit = std::bit_width(K - 1);

  static ExactBound eps() {
    return ExactBound(1, boost::multiprecision::cpp_int(1) << (Kbit * t + 1));
  }

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TLWE<Torus, N>& src,
                        const KeySwitchKey<Torus, n, t, N>& ksk) {
    ExactBound bound = to_exact(tracker->get(src)) +
                       (ExactBound(N * t * (K - 1)) *
                        to_exact(get_key_noise_tracker_if()->get(ksk[0][0]))) +
                       (ExactBound(N) * eps());
    return to_round_up(bound);
  }
};

#endif  // TFHE_NOISE_HPP