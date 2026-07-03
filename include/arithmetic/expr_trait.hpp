// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ARITHMETIC_EXPR_TRAIT_HPP
#define ARITHMETIC_EXPR_TRAIT_HPP

#include <cstdint>

#include "algebra/poly.hpp"

template <typename Expr>
struct evaluated_type;

template <typename Expr>
using evaluated_type_t = typename evaluated_type<Expr>::type;

template <typename T, uint32_t N>
struct evaluated_type<Poly<T, N>> {
  using type = Poly<T, N>;
};

#endif  // ARITHMETIC_EXPR_TRAIT_HPP