// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_PUBLIC_KEY_HPP
#define TFHE_SERIALIZE_PUBLIC_KEY_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/serialize/tlwe.hpp"
#include "tfhe/structure/key/public_key.hpp"

namespace serialize {

template <typename Torus, uint32_t n, uint32_t PkSamples>
struct Serde<PublicKey<Torus, n, PkSamples>> {
  static void write(Writer& w, const PublicKey<Torus, n, PkSamples>& key) {
    for (uint32_t i = 0; i < PkSamples; ++i) {
      Serde<TLWE<Torus, n>>::write(w, key[i]);
    }
  }

  static PublicKey<Torus, n, PkSamples> read(Reader& r) {
    PublicKey<Torus, n, PkSamples> key;
    for (uint32_t i = 0; i < PkSamples; ++i) {
      key[i] = Serde<TLWE<Torus, n>>::read(r);
    }
    return key;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_PUBLIC_KEY_HPP
