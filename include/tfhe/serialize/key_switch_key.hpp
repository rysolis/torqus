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
  static constexpr std::size_t byte_size() {
    return std::size_t{m} * t * Serde<TLWE<Torus, n>>::byte_size();
  }
  static constexpr auto descriptor() {
    return detail::concat(detail::head(TypeTag::KeySwitchKey, n, t, m),
                          Serde<Torus>::descriptor());
  }

  static void serialize(Writer& w, const KeySwitchKey<Torus, n, t, m>& key) {
    for (uint32_t i = 0; i < m; ++i) {
      for (uint32_t j = 0; j < t; ++j) {
        Serde<TLWE<Torus, n>>::serialize(w, key[i][j]);
      }
    }
  }

  static KeySwitchKey<Torus, n, t, m> deserialize(Reader& r) {
    KeySwitchKey<Torus, n, t, m> key;
    for (uint32_t i = 0; i < m; ++i) {
      for (uint32_t j = 0; j < t; ++j) {
        key[i][j] = Serde<TLWE<Torus, n>>::deserialize(r);
      }
    }
    return key;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_KEY_SWITCH_KEY_HPP
