// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TESTVECTOR_HPP
#define TFHE_TESTVECTOR_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/poly.hpp"

#include "e2e/encoding/codec.hpp"
#include "e2e/encoding/message.hpp"

namespace testvector {

// 1/8 + 1/8x + 1/8x^2 + ...
// TODO: use MessageCodec!!!
template <torus_type Torus, uint32_t N>
Poly<Torus, N> generate() {
  return Poly<Torus, N>([] { return 1 << (Torus::qbit - 3); });
}

}  // namespace testvector

#endif  // TFHE_TESTVECTOR_HPP