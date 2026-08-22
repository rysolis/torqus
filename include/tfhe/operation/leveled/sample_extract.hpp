// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SAMPLE_EXTRACT_HPP
#define TFHE_SAMPLE_EXTRACT_HPP

#include "primitive/concept/torus.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace tfhe::leveled {

template <tlwe_concept Lwe, trlwe_concept Rlwe>
class SampleExtract {
 public:
  static constexpr uint32_t N = Rlwe::N;

  template <torus_concept Torus>
  static TLWE<Torus, N> exec_impl(const TRLWE<Torus, N>& trlwe, size_t p) {
    assert(p < N);
    TLWE<Torus, N> tlwe([&trlwe, p](std::size_t i) {
      if (i <= p) {
        return static_cast<Torus>(trlwe.a()[p - i]);
      } else {
        return -static_cast<Torus>(trlwe.a()[N + p - i]);
      }
    });
    tlwe.b() = static_cast<Torus>(trlwe.b()[p]);
    return tlwe;
  }
};

}  // namespace tfhe::leveled

#endif