// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_NOISE_HPP
#define TFHE_NOISE_HPP

#include <cmath>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation.hpp"

template <typename Op>
struct NoisePolicy;

template <trlwe_concept Params>
struct NoisePolicy<tfhe::leveled::Add<Params>> {
  static constexpr uint32_t N = Params::N;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return tracker->get(lhs) + tracker->get(rhs);
  }
};

template <trlwe_concept Params>
struct NoisePolicy<tfhe::leveled::Sub<Params>> {
  static constexpr uint32_t N = Params::N;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return tracker->get(lhs) + tracker->get(rhs);
  }
};

template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct NoisePolicy<tfhe::bootstrap::ExternalProduct<Rlwe, Decomp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr double eps = 1. / (B << (l + 1));

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker, const TRGSW<Torus, N, l>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return (2 * l * N * (B << 1) * tracker->get(lhs)) +
           ((1. / B) * (1 + N) * eps) + ((1. / B) * tracker->get(rhs));
  }
};

template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
struct NoisePolicy<tfhe::bootstrap::CMux<Rlwe, Decomp>> {
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr double eps = (1. / (1u << Bbit * l)) * 0.5;

  template <typename Tracker, torus_concept Torus>
  static double compute(const Tracker* tracker,
                        const TRGSW<Torus, N, l>& selector,
                        const TRLWE<Torus, N>& lhs,
                        const TRLWE<Torus, N>& rhs) {
    return (2 * l * N * (B << 1) * tracker->get(selector)) + ((1 + N) * eps) +
           (std::max(tracker->get(lhs), tracker->get(rhs)));
  }
};

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
  static constexpr double eps = (1. / (1u << Bbit * l)) * 0.5;

  template <typename Tracker>
  static double compute(const Tracker*, const TRLWE<Torus, N>&,
                        const Vector<ModInt<M>, n + 1>&,
                        const BootstrapKey<Torus, N, l, n>& bk) {
    double bound = 0.;  // assume that ||Err(tv)|| = 0
    bound += (n * 2 * l * N * (B << 1) * get_key_noise_tracker_if()->get(bk)) +
             (n * (1 + N) * eps);
    return bound;
  }
};
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
  static constexpr double eps = (1. / (1u << Bbit * l)) * 0.5;

  template <typename Tracker>
  static double compute(const Tracker*, const rTorus, const TRLWE<rTorus, N>&,
                        const TLWE<Torus, n>&,
                        const BootstrapKey<rTorus, N, l, n>& bk) {
    double bound =
        n * 2 * l * N * (B << 1) * get_key_noise_tracker_if()->get(bk) +
        (n * (1 + N) * eps);
    return bound;
  }
};

template <typename Src, typename Dst, typename Kst>
  requires tlwe_concept<Src> && tlwe_concept<Dst> && kst_concept<Kst>
struct NoisePolicy<tfhe::leveled::KeySwitch<Src, Dst, Kst>> {
  static constexpr uint32_t n = Dst::n;
  using Torus = typename Src::torus_type;
  static constexpr uint32_t N = Src::n;
  static constexpr uint32_t K = Kst::K;
  static constexpr uint32_t t = Kst::t;

  template <typename Tracker>
  static double compute(const Tracker* tracker, const TLWE<Torus, N>& src,
                        const KeySwitchKey<Torus, n, t, N>& ksk) {
    double bound = tracker->get(src) +
                   (N * t * get_key_noise_tracker_if()->get(ksk[0][0])) +
                   N / std::pow(2., t + 1);
    return bound;
  }
};

#endif  // TFHE_NOISE_HPP