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