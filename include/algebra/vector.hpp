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

  using Base::begin;
  using Base::end;
  using Base::operator[];

  Vector& operator+=(const Vector& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < other.size(); ++i) {
      (*this)[i] = value_type((*this)[i]) + value_type(other[i]);
    }

    return *this;
  }

  Vector& operator-=(const Vector& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < other.size(); ++i) {
      (*this)[i] = value_type((*this)[i]) - value_type(other[i]);
    }

    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, const Vector& vector) {
    os << "Vec(";
    for (size_t i = 0; i < N; ++i) {
      os << vector[i];
      if (i + 1 != N) os << ", ";
    }
    return os << ")";
  }
};

template <typename T, uint32_t N>
inline Vector<T, N> operator+(Vector<T, N> lhs, const Vector<T, N>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs += rhs;
}

template <typename T, uint32_t N, typename Expr>
inline Vector<T, N> operator+(Vector<T, N> lhs, const Expr& rhs) {
  return lhs += rhs;
}

template <typename T, uint32_t N>
inline Vector<T, N> operator-(Vector<T, N> lhs, const Vector<T, N>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs -= rhs;
}

template <typename T, uint32_t N, typename Expr>
inline Vector<T, N> operator-(Vector<T, N> lhs, const Expr& rhs) {
  return lhs -= rhs;
}

// Proxy<Vector<T, N>> + Proxy<Vector<T, N>> and the corresponding
// subtraction are handled by the generic operator+/operator- for
// Proxy<Container> in detail/proxy.hpp.

#endif  // ALGEBRA_VECTOR_HPP