// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_KEY_SWITCH_KEY_HPP
#define TFHE_SERIALIZE_KEY_SWITCH_KEY_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/serialize/tlwe.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

namespace serialize {

template <typename Torus, uint32_t n, uint32_t t, uint32_t m>
struct Serde<KeySwitchKey<Torus, n, t, m>> {
  static void write(Writer& w, const KeySwitchKey<Torus, n, t, m>& key) {
    for (uint32_t i = 0; i < m; ++i) {
      Serde<Vector<TLWE<Torus, n>, t>>::write(w, key[i]);
    }
  }

  static KeySwitchKey<Torus, n, t, m> read(Reader& r) {
    KeySwitchKey<Torus, n, t, m> key;
    for (uint32_t i = 0; i < m; ++i) {
      key[i] = Serde<Vector<TLWE<Torus, n>, t>>::read(r);
    }
    return key;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_KEY_SWITCH_KEY_HPP
