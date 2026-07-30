// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_NOISE_HPP
#define TFHE_NOISE_HPP

#include <cmath>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation.hpp"

template <typename Op>
struct NoisePolicy;

template <trlwe_concept params>
struct NoisePolicy<Add<params>> {
  static constexpr uint32_t N = params::N;

  template <typename Tracker, torus_type Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return tracker->get(lhs) + tracker->get(rhs);
  }
};

template <trlwe_concept params>
struct NoisePolicy<Sub<params>> {
  static constexpr uint32_t N = params::N;

  template <typename Tracker, torus_type Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return tracker->get(lhs) + tracker->get(rhs);
  }
};

template <typename Rlwe, typename Dcp>
  requires trlwe_concept<Rlwe> && decompose_concept<Dcp>
struct NoisePolicy<ExternalProduct<Rlwe, Dcp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t l = Dcp::l;

  static constexpr double ep = 1. / (B << (l + 1));

  template <typename Tracker, torus_type Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& bk,
                        const TRLWE<Torus, N>& in) {
    return (2 * l * N * (B << 1) * tracker->get(bk)) +
           ((1. / B) * (1 + N) * ep) + ((1. / B) * tracker->get(in));
  }
};

template <typename Rlwe, typename Dcp>
  requires trlwe_concept<Rlwe> && decompose_concept<Dcp>
struct NoisePolicy<CMux<Rlwe, Dcp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t l = Dcp::l;

  static constexpr double ep = (1. / std::pow(B, l)) * 0.5;

  template <typename Tracker, torus_type Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& ml,
                        const TRLWE<Torus, N>& cand0,
                        const TRLWE<Torus, N>& cand1) {
    return (2 * l * N * (B << 1) * tracker->get(ml)) + ((1 + N) * ep) +
           (std::max(tracker->get(cand0), tracker->get(cand1)));
  }
};

template <typename Lwe, typename Rlwe, typename Dcp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Dcp>
struct NoisePolicy<BlindRotate<Lwe, Rlwe, Dcp>> {
  static constexpr uint32_t n = Lwe::n;
  using Torus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;
  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t l = Dcp::l;

  static constexpr double ep = (1. / std::pow(B, l)) * 0.5;

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& tv,
                        const Vector<ModInt<M>, n + 1>&,
                        const BootstrapKey<Torus, N, l, n>& bk) {
    double bound = tracker->get(tv);
    bound +=
        (n * 2 * l * N * (B << 1) * tracker->get(bk[0])) + (n * (1 + N) * ep);
    return bound;
  }
};
template <typename Lwe, typename Rlwe, typename Dcp>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Dcp>
struct NoisePolicy<GateBootstrap<Lwe, Rlwe, Dcp>> {
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;
  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t l = Dcp::l;

  static constexpr double ep = (1. / std::pow(B, l)) * 0.5;

  template <typename Tracker>
  static double compute(const Tracker* tracker, const rTorus,
                        const TRLWE<rTorus, N>&, const TLWE<Torus, n>&,
                        const BootstrapKey<rTorus, N, l, n>& bk) {
    double bound =
        n * 2 * l * N * (B << 1) * tracker->get(bk[0]) + (n * (1 + N) * ep);
    return bound;
  }
};

template <typename Src, typename Dst, typename Kst>
  requires tlwe_concept<Src> && tlwe_concept<Dst> && kst_concept<Kst>
struct NoisePolicy<KeySwitch<Src, Dst, Kst>> {
  static constexpr uint32_t n = Dst::n;
  using Torus = typename Src::torus_type;
  static constexpr uint32_t N = Src::n;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TLWE<Torus, N>& src,
                        const KeySwitchKey<Torus, n, t, N>& ksk) {
    double bound = tracker->get(src) + (N * t * tracker->get(ksk[0][0])) +
                   N / std::pow(2., t + 1);
    return bound;
  }
};

#endif  // TFHE_NOISE_HPP