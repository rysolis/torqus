// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_TRLWE_HPP
#define TFHE_SERIALIZE_TRLWE_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace serialize {

template <typename Torus, uint32_t N>
struct Serde<TRLWE<Torus, N>> {
  static constexpr std::size_t byte_size() {
    return 2 * Serde<Poly<Torus, N>>::byte_size();
  }
  static constexpr auto descriptor() {
    return detail::concat(detail::head(TypeTag::kTrlwe, N),
                          Serde<Torus>::descriptor());
  }

  static void serialize(Writer& w, const TRLWE<Torus, N>& ct) {
    Serde<Poly<Torus, N>>::serialize(w, ct.a());
    Serde<Poly<Torus, N>>::serialize(w, ct.b());
  }

  static TRLWE<Torus, N> deserialize(Reader& r) {
    Poly<Torus, N> a = Serde<Poly<Torus, N>>::deserialize(r);
    Poly<Torus, N> b = Serde<Poly<Torus, N>>::deserialize(r);
    return TRLWE<Torus, N>(a, b);
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_TRLWE_HPP
