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

#include "detail/bulk_arithmetic.hpp"

template <typename T, uint32_t Size>
class Vector : private Container<Vector<T, Size>, T, Size,
                                 storage_traits<T>::use_proxy> {
  using Base =
      Container<Vector<T, Size>, T, Size, storage_traits<T>::use_proxy>;

 public:
  using Base::Base;

  using typename Base::raw_value_type;
  using typename Base::value_type;

  using Base::begin;
  using Base::data;
  using Base::end;
  using Base::operator[];
  using Base::size;

  Vector& operator+=(const Vector& other) {
    assert(other.size() == this->size());

    bulk_add_assign<value_type>(this->data(), other.data(), this->size());

    return *this;
  }

  Vector& operator-=(const Vector& other) {
    assert(other.size() == this->size());

    bulk_sub_assign<value_type>(this->data(), other.data(), this->size());

    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, const Vector& vector) {
    os << "Vec(";
    for (size_t i = 0; i < Size; ++i) {
      os << vector[i];
      if (i + 1 != Size) os << ", ";
    }
    return os << ")";
  }
};

template <typename T, uint32_t Size>
inline Vector<T, Size> operator+(Vector<T, Size> lhs,
                                 const Vector<T, Size>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs += rhs;
}

template <typename T, uint32_t Size, typename Expr>
inline Vector<T, Size> operator+(Vector<T, Size> lhs, const Expr& rhs) {
  return lhs += rhs;
}

template <typename T, uint32_t Size>
inline Vector<T, Size> operator-(Vector<T, Size> lhs,
                                 const Vector<T, Size>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs -= rhs;
}

template <typename T, uint32_t Size, typename Expr>
inline Vector<T, Size> operator-(Vector<T, Size> lhs, const Expr& rhs) {
  return lhs -= rhs;
}

// Proxy<Vector<T, Size>> + Proxy<Vector<T, Size>> and the corresponding
// subtraction are handled by the generic operator+/operator- for
// Proxy<Container> in detail/proxy.hpp.

#endif  // ALGEBRA_VECTOR_HPP