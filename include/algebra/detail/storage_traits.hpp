// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_DETAIL_STORAGE_TRAITS_HPP
#define ALGEBRA_DETAIL_STORAGE_TRAITS_HPP

#include <type_traits>

#include "primitive/concept/primitive.hpp"

// storage_traits<T> is only defined for primitive_concept T exposing a
// raw_value_type -- the numeric types (ModTorus, ModInt, UInt, ...)
// Vector<T,Size>/Poly<T,Size>'s flat-buffer storage is built for.
// Composite/aggregate T (TLWE, TRLWE, Cipher, ...) has no specialization
// here and so can't be a Vector<T,Size> element -- use std::vector<T>
// for those instead (see e.g. trgsw.hpp's own trlwe_rows_).
template <typename T, typename = void>
struct storage_traits;

template <primitive_concept T>
struct storage_traits<T, std::void_t<typename T::raw_value_type>> {
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;
  static constexpr bool use_proxy = !std::same_as<T, raw_value_type>;
};

#endif  // ALGEBRA_DETAIL_STORAGE_TRAITS_HPP
