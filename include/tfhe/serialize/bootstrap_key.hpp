// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_BOOTSTRAP_KEY_HPP
#define TFHE_SERIALIZE_BOOTSTRAP_KEY_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/serialize/trgsw.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

namespace serialize {

template <typename Torus, uint32_t N, uint32_t l, uint32_t n>
struct Serde<BootstrapKey<Torus, N, l, n>> {
  static void serialize(Writer& w, const BootstrapKey<Torus, N, l, n>& key) {
    for (uint32_t i = 0; i < n; ++i) {
      Serde<TRGSW<Torus, N, l>>::serialize(w, key[i]);
    }
  }

  static BootstrapKey<Torus, N, l, n> deserialize(Reader& r) {
    BootstrapKey<Torus, N, l, n> key;
    for (uint32_t i = 0; i < n; ++i) {
      key[i] = Serde<TRGSW<Torus, N, l>>::deserialize(r);
    }
    return key;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_BOOTSTRAP_KEY_HPP
