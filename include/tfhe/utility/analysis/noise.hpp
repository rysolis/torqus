// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_NOISE_HPP
#define TFHE_NOISE_HPP

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

template <decompose_concept params>
struct NoisePolicy<ExternalProduct<params>> {
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t B = params::B;
  static constexpr uint32_t l = params::l;

  static constexpr double ep = 1. / (B << (l + 1));

  template <typename Tracker, torus_type Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& bk,
                        const TRLWE<Torus, N>& in) {
    return (2 * l * N * (B << 1) * tracker->get(bk)) +
           ((1. / B) * (1 + N) * ep) + ((1. / B) * tracker->get(in));
  }
};

template <decompose_concept params>
struct NoisePolicy<CMux<params>> {
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t B = params::B;
  static constexpr uint32_t l = params::l;

  static constexpr double ep = 1. / (B << (l + 1));

  template <typename Tracker, torus_type Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& ml,
                        const TRLWE<Torus, N>& cand0,
                        const TRLWE<Torus, N>& cand1) {
    return (2 * l * N * (B << 1) * tracker->get(ml)) +
           ((1. / B) * (1 + N) * ep) +
           ((1. / B) * std::max(tracker->get(cand0), tracker->get(cand1)));
  }
};

template <tlwe_concept fparams, trlwe_concept bparams>
  requires decompose_concept<bparams>
struct NoisePolicy<BlindRotate<fparams, bparams>> {
  using bTorus = typename bparams::torus_type;

  static constexpr uint32_t n = fparams::n;
  static constexpr uint32_t N = bparams::N;
  static constexpr uint32_t B = bparams::B;
  static constexpr uint32_t l = bparams::l;
  static constexpr uint32_t M = 2 * N;

  static constexpr double ep = 1. / (B << (l + 1));

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TRLWE<bTorus, N>& tv,
                        const Vector<ModInt<M>, n + 1>&,
                        const BootstrapKey<bTorus, N, l, n>& bk) {
    double bound = tracker->get(tv);
    for (size_t i = 0; i < n; ++i) {
      bound += (2 * l * N * (B << 1) * tracker->get(bk[i])) +
               ((1. / B) * (1 + N) * ep) +
               ((1. / B) * std::max(tracker->get(tv), tracker->get(tv)));
    }
    return bound;
  }
};

template <tlwe_concept fparams, tlwe_concept bparams>
  requires key_switch_concept<bparams>
struct NoisePolicy<KeySwitch<fparams, bparams>> {
  static constexpr uint32_t n = fparams::n;

  using bTorus = typename bparams::torus_type;
  static constexpr uint32_t N = bparams::n;
  static constexpr uint32_t K = bparams::K;
  static constexpr uint32_t t = bparams::t;

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TLWE<bTorus, N>& src,
                        const KeySwitchKey<bTorus, n, t, N>& ksk) {
    double bound = tracker->get(src) + (N * t * tracker->get(ksk[0][0])) +
                   N / std::pow(2., t + 1);
    return bound;
  }
};

#endif  // TFHE_NOISE_HPP