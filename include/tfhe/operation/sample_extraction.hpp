// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SAMPLE_EXTRACTION_HPP
#define TFHE_SAMPLE_EXTRACTION_HPP

#include "primitive/concept/torus.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <tlwe_concept fparams, trlwe_concept bparams>
class SampleExtraction {
 public:
  static constexpr uint32_t N = bparams::N;

  template <torus_type Torus>
  static TLWE<Torus, N> exec(const TRLWE<Torus, N>& trlwe, size_t p) {
    TLWE<Torus, N> tlwe([&trlwe, p](std::size_t i) {
      if (i <= p) {
        return static_cast<Torus>(trlwe.a()[i]);
      } else {
        return static_cast<Torus>(-trlwe.a()[N + p - i]);
      }
    });
    tlwe.b() = static_cast<Torus>(trlwe.b()[p]);
    return tlwe;
  }
};

#endif