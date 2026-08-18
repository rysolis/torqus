// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_TRLWE_HPP
#define TFHE_SERIALIZE_TRLWE_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace serialize {

template <typename Torus, uint32_t N>
struct Serde<TRLWE<Torus, N>> {
  static void write(Writer& w, const TRLWE<Torus, N>& ct) {
    Serde<Poly<Torus, N>>::write(w, ct.a());
    Serde<Poly<Torus, N>>::write(w, ct.b());
  }

  static TRLWE<Torus, N> read(Reader& r) {
    Poly<Torus, N> a = Serde<Poly<Torus, N>>::read(r);
    Poly<Torus, N> b = Serde<Poly<Torus, N>>::read(r);
    return TRLWE<Torus, N>(a, b);
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_TRLWE_HPP
