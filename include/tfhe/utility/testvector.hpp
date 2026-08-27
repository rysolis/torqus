// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TESTVECTOR_HPP
#define TFHE_TESTVECTOR_HPP

#include <cstdint>

#include "primitive/concept/torus.hpp"

#include "algebra/poly.hpp"

namespace testvector {

// -c + -cx + -cx^2 + ... + cx^{N/2} + cx^{N/2+1} + ...
// TODO: use MessageCodec!!!
template <torus_concept Torus, uint32_t N>
Poly<Torus, N> generate(Torus c) {
  return Poly<Torus, N>([c](uint32_t i) {
    if (i >= (N / 2)) {
      return c;
    } else {
      return -c;
    }
  });
}

}  // namespace testvector

#endif  // TFHE_TESTVECTOR_HPP