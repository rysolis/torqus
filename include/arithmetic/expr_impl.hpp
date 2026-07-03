// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ARITHMETIC_EXPR_IMPL_HPP
#define ARITHMETIC_EXPR_IMPL_HPP

#include <cstdint>

#include "algebra/poly.hpp"

template <typename Expr>
struct accumulate_impl;

template <typename T, uint32_t N>
template <typename Expr, typename AddOp, typename SubOp>
void Poly<T, N>::accumulate_expr(const Expr& ep, AddOp add_op, SubOp sub_op) {
  accumulate_impl<Expr>::apply(*this, ep, add_op, sub_op);
}

#endif  // ARITHMETIC_EXPR_IMPL_HPP