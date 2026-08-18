// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_TLWE_HPP
#define TFHE_SERIALIZE_TLWE_HPP

#include "tfhe/serialize/serde.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

namespace serialize {

template <typename Torus, uint32_t n>
struct Serde<TLWE<Torus, n>> {
  static void write(Writer& w, const TLWE<Torus, n>& ct) {
    Serde<Vector<Torus, n>>::write(w, ct.a());
    Serde<Torus>::write(w, ct.b());
  }

  static TLWE<Torus, n> read(Reader& r) {
    TLWE<Torus, n> ct;
    ct.a() = Serde<Vector<Torus, n>>::read(r);
    ct.b() = Serde<Torus>::read(r);
    return ct;
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_TLWE_HPP
