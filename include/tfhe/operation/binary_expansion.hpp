// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <cstdint>

#include "algebra/vector.hpp"

#include "tfhe/operation/hom_and.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) input bit-ciphertexts.
template <uint32_t H, typename Lwe, typename Rlwe, typename Decomp>
class BinaryExpansion {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  static constexpr uint32_t k = std::bit_width(H - 1);

  static Vector<TLWE<rTorus, N>, H> exec_impl(
      const Vector<TLWE<Torus, n>, k>& v,
      const BootstrapKey<rTorus, N, l, n>& bk) {
    Vector<TLWE<rTorus, N>, H> res;
    for (size_t h = 0; h < H; ++h) {
      TLWE<rTorus, N> w;
      w.b() = rTorus(1u, 4u);
      for (size_t i = 0; i < k; ++i) {
        uint32_t bit = (h >> i) & 1u;
        w = [&] {
          return bit ? HomAnd<Lwe, Rlwe, Decomp>::exec_impl(w, v[i], bk)
                     : HomAndNot<Lwe, Rlwe, Decomp>::exec_impl(w, v[i], bk);
        }();
      }
      res[h] = std::move(w);
    }
    return res;
  }
};

#endif