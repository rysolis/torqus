// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_PARAMS_HPP
#define TFHE_PARAMS_HPP

#include <cstdint>

#include "tfhe/concept/tfhe.hpp"

template <typename Torus, uint32_t dim>
struct tlwe_core_params {
  using torus_type = Torus;
  static constexpr uint32_t n = dim;
};

template <typename Torus, uint32_t dim>
struct trlwe_core_params {
  using torus_type = Torus;
  static constexpr uint32_t N = dim;
};

template <uint32_t Base, uint32_t Digit>
struct dcp_params {
  static constexpr uint32_t B = Base;
  static constexpr uint32_t l = Digit;
};

// For key switch
template <uint32_t Base, uint32_t Digit>
struct kst_params {
  static constexpr uint32_t K = Base;
  static constexpr uint32_t t = Digit;
};

// Fresh-encryption noise standard deviation, alpha = 2^-AlphaBits (a power
// of two, matching this file's other params' preference for exact
// power-of-two magnitudes). Mix into lwe_params/rlwe_params's Features
// alongside dcp_params/kst_params to opt a param set into real Gaussian
// noise sampling at encryption; see alpha_of below for what happens when
// it's omitted.
template <uint32_t AlphaBits>
struct noise_params {
  static constexpr double alpha =
      1.0 / static_cast<double>(uint64_t{1} << AlphaBits);
};

// alpha_of<Params>::value is Params::alpha when noise_params was mixed in,
// or 0.0 otherwise -- so encryption code can unconditionally ask for
// alpha_of<Params>::value without every Params (including every test
// fixture's own toy params) needing to opt into noise_params explicitly.
//
// Defining TFHE_DISABLE_NOISE (see CMakeLists.txt's TFHE_ENABLE_NOISE
// option) drops the specialization below entirely, so alpha_of<Params> is
// always 0 for every Params regardless of noise_params -- a single
// compile-time switch back to exact/noiseless ciphertexts everywhere.
template <typename Params>
struct alpha_of {
  static constexpr double value = 0.0;
};
#ifndef TFHE_DISABLE_NOISE
template <typename Params>
  requires requires { Params::alpha; }
struct alpha_of<Params> {
  static constexpr double value = Params::alpha;
};
#endif  // TFHE_DISABLE_NOISE

// Deterministic worst-case bound assigned to a freshly-sampled Gaussian(0,
// alpha^2) error, for tfhe/utility/analysis/tracker_if.hpp's NoiseTracker:
// Pr[|e| > 6*alpha] =~ 3e-8, the same order of magnitude as
// utility/analysis/noise.hpp's own decomposition eps() terms (2^-25 ..
// 2^-33), so cutting the tail off here doesn't introduce a new dominant
// source of (negligible) failure probability. Lives here rather than in
// noise.hpp since it depends only on alpha_of<Params> above, not on
// NoisePolicy/ExactBound -- keeps runtime.hpp's dependency on this file
// (already needed for Params itself) from having to pull in the whole
// Op-noise-analysis machinery just to report a fresh ciphertext's bound.
inline constexpr double kNoiseTailSigma = 6.0;

template <typename Params>
double fresh_noise_bound() {
  return kNoiseTailSigma * alpha_of<Params>::value;
}

template <typename Core, typename... Features>
struct lwe_params : Core, Features... {};

template <typename Core, typename... Features>
struct rlwe_params : Core, Features... {};

template <typename... Params>
struct ParamsPack : Params... {};

// The Lwe-shaped view of an Rlwe's own ciphertext space (torus_type =
// Rlwe's, n = Rlwe::N). SampleExtract turns a TRLWE(S) ciphertext into a
// TLWE ciphertext under secret = coeffs(S), of exactly this shape; naming
// it here (rather than where a specific caller needs it) keeps it usable
// by any core operation -- e.g. BinaryExpansion's own internal KeySwitch --
// without that operation depending on a higher layer to supply it.
template <typename Rlwe>
  requires trlwe_concept<Rlwe>
using ExtractedLwe =
    lwe_params<tlwe_core_params<typename Rlwe::torus_type, Rlwe::N>>;

template <typename Message, typename Codec>
struct encoding_params {
  using message_type = Message;
  using codec_type = Codec;
};

#endif  // TFHE_PARAMS_HPP