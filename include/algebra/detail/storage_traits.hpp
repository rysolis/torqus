// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_DETAIL_STORAGE_TRAITS_HPP
#define ALGEBRA_DETAIL_STORAGE_TRAITS_HPP

#include <type_traits>

#include "primitive/concept/primitive.hpp"

template <typename T, typename = void>
struct storage_traits {
  using value_type = T;
  using raw_value_type = T;
  static constexpr bool use_proxy = false;
};

template <primitive_concept T>
struct storage_traits<T, std::void_t<typename T::raw_value_type>> {
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;
  static constexpr bool use_proxy = !std::same_as<T, raw_value_type>;
};

#endif  // ALGEBRA_DETAIL_STORAGE_TRAITS_HPP
