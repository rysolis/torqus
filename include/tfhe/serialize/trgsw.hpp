// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_TRGSW_HPP
#define TFHE_SERIALIZE_TRGSW_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/serialize/trlwe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"

namespace serialize {

template <typename Torus, uint32_t N, uint32_t l>
struct Serde<TRGSW<Torus, N, l>> {
  static void serialize(Writer& w, const TRGSW<Torus, N, l>& key) {
    for (uint32_t i = 0; i < key.level(); ++i) {
      Serde<TRLWE<Torus, N>>::serialize(w, key[i]);
    }
  }

  static TRGSW<Torus, N, l> deserialize(Reader& r) {
    TRGSW<Torus, N, l> key;
    for (uint32_t i = 0; i < key.level(); ++i) {
      key[i] = Serde<TRLWE<Torus, N>>::deserialize(r);
    }
    return key;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_TRGSW_HPP
