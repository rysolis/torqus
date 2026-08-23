// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_POLY_HPP
#define ALGEBRA_POLY_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <ranges>
#include <type_traits>

#include "primitive/concept/convertible.hpp"

#include "algebra/container.hpp"

#include "detail/bulk_arithmetic.hpp"
#include "detail/proxy.hpp"

struct expr_tag {};

template <typename Expr>
struct accumulate_impl;

template <typename T, uint32_t Size>
class Poly : private Container<Poly<T, Size>, T, Size, true> {
  using Base = Container<Poly<T, Size>, T, Size, true>;

 public:
  using Base::Base;

  using typename Base::raw_value_type;
  using typename Base::value_type;

  using Base::begin;
  using Base::data;
  using Base::end;
  using Base::operator[];
  using Base::size;

  Poly& operator+=(const Poly& other) {
    assert(other.size() == this->size());

    bulk_add_assign<value_type>(this->data(), other.data(), this->size());
    return *this;
  }

  Poly& operator-=(const Poly& other) {
    assert(other.size() == this->size());

    bulk_sub_assign<value_type>(this->data(), other.data(), this->size());
    return *this;
  }

  template <typename Expr>
    requires requires(Expr& expr) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly(const Expr& expr) {
    std::fill(begin(), end(), typename T::raw_value_type{});
    accumulate_expr(
        expr,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        });
  }

  template <typename Expr>
    requires requires(Expr& expr) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly& operator=(const Expr& expr) {
    accumulate_expr(
        expr,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        });
    return *this;
  }

  template <typename Expr>
    requires requires(Expr& expr) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly& operator+=(const Expr& expr) {
    accumulate_expr(
        expr,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        });
    return *this;
  }

  template <typename Expr>
    requires requires(Expr& expr) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly& operator-=(const Expr& expr) {
    accumulate_expr(
        expr,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        });
    return *this;
  }

  constexpr bool operator==(const Poly& other) const noexcept {
    if (other.size() != this->size()) {
      return false;
    }
    return std::ranges::equal(*this, other);
  }

  [[nodiscard]] raw_value_type* release() { return this->release_buffer(); }

  friend std::ostream& operator<<(std::ostream& os, const Poly& poly) {
    os << "Poly(";
    for (size_t i = 0; i < poly.size(); ++i) {
      os << poly[i];
      if (i + 1 < poly.size()) {
        os << ", ";
      }
    }
    os << ")";
    return os;
  }

 private:
  template <typename Expr, typename AddOp, typename SubOp>
  void accumulate_expr(const Expr& expr, AddOp add_op, SubOp sub_op) {
    accumulate_impl<Expr>::apply(*this, expr, add_op, sub_op);
  }
};

template <typename T, uint32_t Size>
inline Poly<T, Size> operator+(Poly<T, Size> lhs, const Poly<T, Size>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs += rhs;
}

template <typename T, uint32_t Size, typename Expr>
inline Poly<T, Size> operator+(Poly<T, Size> lhs, const Expr& rhs) {
  return lhs += rhs;
}

template <typename T, uint32_t Size>
inline Poly<T, Size> operator-(Poly<T, Size> lhs, const Poly<T, Size>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs -= rhs;
}

template <typename T, uint32_t Size, typename Expr>
inline Poly<T, Size> operator-(Poly<T, Size> lhs, const Expr& rhs) {
  return lhs -= rhs;
}

// Proxy<Poly<T, Size>> + Proxy<Poly<T, Size>> and the corresponding subtraction
// are handled by the generic operator+/operator- for Proxy<Container> in
// detail/proxy.hpp.

#endif  // ALGEBRA_POLY_HPP
