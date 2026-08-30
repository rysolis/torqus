// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_VARIANCE_NOISE_HPP
#define TFHE_VARIANCE_NOISE_HPP

#include <algorithm>
#include <boost/math/distributions/normal.hpp>
#include <cmath>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"

// Variance-based ("average-case") noise analysis, alongside noise.hpp's
// worst-case NoisePolicy -- not a replacement for it. Every formula here is
// Chillotti et al.'s own average-case corollary/lemma/theorem ("TFHE: Fast
// Fully Homomorphic Encryption over the Torus", eprint.iacr.org/2018/421,
// the same paper noise.hpp's NoisePolicy<KeySwitch> already cites),
// specialized to this codebase's k=1 (single-mask TRLWE/TRGSW) convention,
// and generalized from the paper's binary decomposition to this codebase's
// arbitrary base B/K exactly the way noise.hpp's own NoisePolicy<KeySwitch>
// already generalizes the worst-case formula (see its NOTE on provenance):
// by applying the paper's own Fact 2.2 (Var(c*X) = c^2 * Var(X) for a
// linear combination with constant c) to whatever the worst-case bound's
// linear digit-magnitude factor -- (B>>1) or (K-1) -- was, squaring it.
//
// Unlike NoisePolicy, these numbers are a statistical prediction under the
// paper's own Independence Heuristic (Assumption 3.11: every decomposition
// digit and every input ciphertext's noise is an independent, mean-zero
// (sub)gaussian random variable), not a provable hard bound -- so unlike
// NoisePolicy, trusting it for a real ciphertext requires empirical
// validation (see gate_bootstrap_test.cpp's single-shot high-confidence
// check for an example), not just a formula-correctness unit test.
//
// Runtime::encrypt()/generate_bootstrap_key()/generate_key_switch_key() and
// Evaluator<Op, Tracking>::exec() (evaluator.hpp) already call these
// specializations and register the result via get_variance_tracker_if()/
// get_key_variance_tracker_if(), the same way they already do for
// NoisePolicy's worst-case bound -- so most callers never need to name
// VarianceNoisePolicy directly at all, just read the tracker back.
//
// Tracker arguments carry VARIANCES here (Var(Err(ct))), never the
// worst-case bounds NoisePolicy's Tracker carries -- same
// NoiseTrackerInterface shape (get/update keyed by ciphertext identity),
// different semantic contract, and (since variance actually composes by
// plain addition, unlike a worst-case bound that needs the exact-rational
// bookkeeping ExactBound exists for) a plain double is enough here. Where
// NoisePolicy reads a key's worst-case bound off get_key_noise_tracker_if(),
// the operations below read its variance off get_key_variance_tracker_if()
// (tracker_if.hpp) the same way -- a distinct singleton, not the same one
// under a different name (see tracker_if_impl.hpp's own note on why each
// tracker role needs its own slot).
template <typename Op>
struct VarianceNoisePolicy;

template <trlwe_concept Params>
struct VarianceNoisePolicy<tfhe::leveled::Add<Params>> {
  static constexpr uint32_t N = Params::N;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return tracker->get(lhs) + tracker->get(rhs);
  }
};

template <trlwe_concept Params>
struct VarianceNoisePolicy<tfhe::leveled::Sub<Params>> {
  static constexpr uint32_t N = Params::N;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return tracker->get(lhs) + tracker->get(rhs);
  }
};

// Corollary 3.14 (Average-case External Product), k=1:
//   Var(Err(A box b)) <= (k+1)*l*N*beta^2*Var(Err(A))
//                        + (1+kN)*||muA||_2^2*eps^2 + ||muA||_2^2*Var(Err(b))
// with beta=B/2 and ||muA||_2^2 <= 1 for a bit-message TRGSW (matching
// NoisePolicy's own worst-case ||muA||_1 <= 1 assumption).
template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct VarianceNoisePolicy<tfhe::bootstrap::ExternalProduct<Rlwe, Decomp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static double eps() {
    return std::ldexp(1.0, -static_cast<int>(Bbit * l + 1));
  }

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    double beta = static_cast<double>(B) / 2.0;
    return 2.0 * l * N * (beta * beta) * tracker->get(lhs) +
           (1 + N) * eps() * eps() + tracker->get(rhs);
  }
};

// Lemma 3.16 (CMux gate), k=1:
//   Var(Err(CMux(C,d1,d0))) <= max(Var(Err(d0)),Var(Err(d1))) + vartheta(C)
//   vartheta(C) = (k+1)*l*N*beta^2*Var(Err(C)) + (kN+1)^2*eps^2
// Note the (N+1)^2 here versus Corollary 3.14's linear (1+N) -- both are
// exactly as the paper states them; see this file's own top-of-file note.
template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct VarianceNoisePolicy<tfhe::bootstrap::CMux<Rlwe, Decomp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static double eps() {
    return std::ldexp(1.0, -static_cast<int>(Bbit * l + 1));
  }

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker,
                        const TRGSW<Torus, N, l>& selector,
                        const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    double beta = static_cast<double>(B) / 2.0;
    double vartheta = 2.0 * l * N * (beta * beta) * tracker->get(selector) +
                      (N + 1.0) * (N + 1.0) * eps() * eps();
    return std::max(tracker->get(lhs), tracker->get(rhs)) + vartheta;
  }
};

// Theorem 4.3 (BlindRotate), k=1, p=n CMux rounds -- inherits Lemma 3.16's
// per-round vartheta(C) verbatim, summed n times (the accumulator's own tv
// noise is assumed 0, matching NoisePolicy<BlindRotate>'s same assumption):
//   Var(Err(ACC)) <= n*(k+1)*l*N*beta^2*Var(Err(bk)) + n*(kN+1)^2*eps^2
template <typename Lwe, typename Rlwe, typename Decomp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct VarianceNoisePolicy<tfhe::bootstrap::BlindRotate<Lwe, Rlwe, Decomp>> {
  static constexpr uint32_t n = Lwe::n;
  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static double eps() {
    return std::ldexp(1.0, -static_cast<int>(Bbit * l + 1));
  }

  template <typename Tracker>
  static double compute(const Tracker*, const TRLWE<Torus, N>&,
                        const Vector<ModInt<M>, n + 1>&,
                        const BootstrapKey<Torus, N, l, n>& bk) {
    double beta = static_cast<double>(B) / 2.0;
    double bk_key_variance = get_key_variance_tracker_if()->get(bk);
    return static_cast<double>(n) * 2.0 * l * N * (beta * beta) *
               bk_key_variance +
           n * (N + 1.0) * (N + 1.0) * eps() * eps();
  }
};

// GateBootstrap's own noise contribution is exactly BlindRotate's (its
// Add(offset, SampleExtract(rot)) step is otherwise noise-free), matching
// NoisePolicy<GateBootstrap>'s identical relationship to
// NoisePolicy<BlindRotate>.
template <typename Lwe, typename Rlwe, typename Decomp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct VarianceNoisePolicy<tfhe::bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>> {
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  static double eps() {
    return std::ldexp(1.0, -static_cast<int>(Bbit * l + 1));
  }

  template <typename Tracker>
  static double compute(const Tracker*, const rTorus, const TRLWE<rTorus, N>&,
                        const TLWE<Torus, n>&,
                        const BootstrapKey<rTorus, N, l, n>& bk) {
    double beta = static_cast<double>(B) / 2.0;
    double bk_key_variance = get_key_variance_tracker_if()->get(bk);
    return static_cast<double>(n) * 2.0 * l * N * (beta * beta) *
               bk_key_variance +
           n * (N + 1.0) * (N + 1.0) * eps() * eps();
  }
};

// Theorem 4.1 (Public Key Switching), generalized from the paper's binary
// decomposition to this codebase's arbitrary base K the same way
// NoisePolicy<KeySwitch>'s worst-case bound already is (see its NOTE on
// provenance): the paper's average case is
//   Var(Err(c)) <= R^2*Var(Err(cin)) + n*t*N_out*vartheta_KS +
//   N_out*n*2^-2(t+1)
// with R=1 (KeySwitch here evaluates the identity function on one input)
// and N_out=1 (TLWE-to-TLWE, per the paper's own Remark 2). Each of the
// N*t decomposition-digit x KSK-row terms has digit magnitude <= (K-1) --
// binary decomposition's implicit digit<=1 is exactly K=2's (K-1)=1 case --
// so by Fact 2.2 (Var(c*X)=c^2*Var(X)) their combined variance gets the
// squared (K-1)^2 factor, mirroring the worst-case bound's linear (K-1).
template <typename Src, typename Dst, typename Kst>
  requires tlwe_concept<Src> && tlwe_concept<Dst> && kst_concept<Kst>
struct VarianceNoisePolicy<tfhe::leveled::KeySwitch<Src, Dst, Kst>> {
  static constexpr uint32_t n = Dst::n;
  using Torus = typename Src::torus_type;
  static constexpr uint32_t N = Src::n;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  static constexpr uint32_t Kbit = std::bit_width(K - 1);

  static double eps() {
    return std::ldexp(1.0, -static_cast<int>(Kbit * t + 1));
  }

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TLWE<Torus, N>& src,
                        const KeySwitchKey<Torus, n, t, N>& ksk) {
    double digit = static_cast<double>(K) - 1.0;
    double ksk_key_variance = get_key_variance_tracker_if()->get(ksk[0][0]);
    return tracker->get(src) + N * t * (digit * digit) * ksk_key_variance +
           N * eps() * eps();
  }
};

// Declared in tracker_if.hpp; defined here since it needs boost::math. The
// Bonferroni bound reduces to the ordinary single-sample Gaussian z (~2.576)
// at coefficient_count=1.
inline double gaussian_estimate_for_max_of(uint32_t coefficient_count) {
  boost::math::normal_distribution<double> standard_normal(0.0, 1.0);
  double tail_probability = 0.005 / static_cast<double>(coefficient_count);
  return boost::math::quantile(
      boost::math::complement(standard_normal, tail_probability));
}

#endif  // TFHE_VARIANCE_NOISE_HPP
