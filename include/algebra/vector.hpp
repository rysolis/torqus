// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_VECTOR_HPP
#define ALGEBRA_VECTOR_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>

#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"

#include "algebra/container.hpp"

template <typename T, uint32_t N>
class Vector
    : public Container<Vector<T, N>, T, N, storage_traits<T>::use_proxy> {
  using Base = Container<Vector<T, N>, T, N, storage_traits<T>::use_proxy>;

 public:
  using Base::Base;

  using typename Base::raw_value_type;
  using typename Base::value_type;

  using Base::operator[];

  friend std::ostream& operator<<(std::ostream& os, const Vector& vec) {
    os << "Vec(";
    for (size_t i = 0; i < N; ++i) {
      os << vec[i];
      if (i + 1 != N) os << ", ";
    }
    return os << ")";
  }
};

// If operator+(Proxy lhs, const Proxy<T>& rhs) is used,
// lhs is copied together with its reference state,
// which results in unintended behavior.
template <typename T, uint32_t N>
inline Vector<T, N>::value_type operator+(
    const typename Vector<T, N>::Proxy& lhs,
    const typename Vector<T, N>::Proxy& rhs) {
  return static_cast<Vector<T, N>::raw_value_type>(lhs) +
         static_cast<Vector<T, N>::raw_value_type>(rhs);
}

// If operator-(Proxy lhs, const Proxy<T>& rhs) is used,
// lhs is copied together with its reference state,
// which results in unintended behavior.
template <typename T, uint32_t N>
inline Vector<T, N>::value_type operator-(
    const typename Vector<T, N>::Proxy& lhs,
    const typename Vector<T, N>::Proxy& rhs) {
  return static_cast<Vector<T, N>::raw_value_type>(lhs) -
         static_cast<Vector<T, N>::raw_value_type>(rhs);
}

#endif  // ALGEBRA_VECTOR_HPP