// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_TLWE_HPP
#define TFHE_SERIALIZE_TLWE_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

namespace serialize {

template <typename Torus, uint32_t n>
struct Serde<TLWE<Torus, n>> {
  static constexpr std::size_t byte_size() {
    return Serde<Vector<Torus, n>>::byte_size() + Serde<Torus>::byte_size();
  }
  static constexpr auto descriptor() {
    return detail::concat(detail::head(TypeTag::kTlwe, n),
                          Serde<Torus>::descriptor());
  }

  static void serialize(Writer& w, const TLWE<Torus, n>& ct) {
    Serde<Vector<Torus, n>>::serialize(w, ct.a());
    Serde<Torus>::serialize(w, ct.b());
  }

  static TLWE<Torus, n> deserialize(Reader& r) {
    TLWE<Torus, n> ct;
    ct.a() = Serde<Vector<Torus, n>>::deserialize(r);
    ct.b() = Serde<Torus>::deserialize(r);
    return ct;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_TLWE_HPP
