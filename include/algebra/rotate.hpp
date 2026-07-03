// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_ROTATE_HPP
#define ALGEBRA_ROTATE_HPP

#include <cstdint>

#include "algebra/poly.hpp"

// COPY OCCURED!!!
template <typename T, uint32_t N>
Poly<T, N> rotate(const Poly<T, N>& p, uint32_t m) {
  constexpr uint32_t M = 2 * N;
  m %= M;

  return Poly<T, N>([&p, m](uint32_t i) {
    uint32_t j = (i + M - m) % N;
    bool neg = ((j + m) / N) & 1;
    return neg ? -p[j] : p[j];
  });
}

#endif  // ALGEBRA_ROTATE_HPP