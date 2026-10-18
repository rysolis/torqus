// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_PUBLIC_KEY_HPP
#define TFHE_SERIALIZE_PUBLIC_KEY_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/structure/key/public_key.hpp"

namespace serialize {

// Serde<Ciphertext> must already be visible at the instantiation site --
// serialize/tlwe.hpp, serialize/trlwe.hpp or serialize/trgsw.hpp, matching
// whichever shape PublicKey's own samples are (see public_key.hpp).
template <typename Ciphertext, uint32_t PkSamples>
struct Serde<PublicKey<Ciphertext, PkSamples>> {
  static void write(Writer& w, const PublicKey<Ciphertext, PkSamples>& key) {
    for (uint32_t i = 0; i < PkSamples; ++i) {
      Serde<Ciphertext>::write(w, key[i]);
    }
  }

  static PublicKey<Ciphertext, PkSamples> read(Reader& r) {
    PublicKey<Ciphertext, PkSamples> key;
    for (uint32_t i = 0; i < PkSamples; ++i) {
      key[i] = Serde<Ciphertext>::read(r);
    }
    return key;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_PUBLIC_KEY_HPP
