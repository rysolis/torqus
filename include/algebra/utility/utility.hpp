// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_UTILITY_HPP
#define ALGEBRA_UTILITY_HPP

#include "primitive/concept/convertible.hpp"
#include "primitive/concept/interpretable.hpp"
#include "primitive/concept/torus.hpp"

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

template <torus_type Torus, uint32_t N>
inline constexpr double infinity_norm(const Vector<Torus, N>& poly) {
  double max_norm = 0.0;
  for (size_t i = 0; i < poly.size(); ++i) {
    double abs_val = infinity_norm(static_cast<Torus>(poly[i]));
    if (abs_val > max_norm) {
      max_norm = abs_val;
    }
  }
  return max_norm;
}

template <typename To, typename From, uint32_t N>
  requires explicitly_convertible_to<To, From>
inline Vector<To, N> convert_to(const Vector<From, N>& src) {
  return Vector<To, N>([&](size_t i) {
    return static_cast<To>(static_cast<typename To::raw_value_type>(src[i]));
  });
}

template <torus_type Torus, uint32_t N>
inline constexpr double infinity_norm(const Poly<Torus, N>& poly) {
  double max_norm = 0.0;
  for (size_t i = 0; i < poly.size(); ++i) {
    double abs_val = infinity_norm(static_cast<Torus>(poly[i]));
    if (abs_val > max_norm) {
      max_norm = abs_val;
    }
  }
  return max_norm;
}

template <typename To, typename From, uint32_t N>
  requires explicitly_convertible_to<To, From>
inline Poly<To, N> convert_to(const Poly<From, N>& src) {
  return Poly<To, N>([&](size_t i) {
    return static_cast<To>(static_cast<typename To::raw_value_type>(src[i]));
  });
}

template <typename To, typename From, uint32_t N>
  requires interpretable_to<To, From>
Poly<To, N> interpret_as(const Poly<From, N>& src) {
  return Poly<To, N>(src.begin(), src.end());
}

template <typename To, typename From, uint32_t N>
  requires interpretable_to<To, From>
Poly<To, N> interpret_as(Poly<From, N>&& src) {
  return Poly<To, N>(src.release());
}

#endif  // ALGEBRA_UTILITY_HPP