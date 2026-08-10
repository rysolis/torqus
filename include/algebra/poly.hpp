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

#include "detail/proxy.hpp"

struct expr_tag {};

template <typename Expr>
struct accumulate_impl;

template <typename T, uint32_t N>
class Poly : public Container<Poly<T, N>, T, N, true> {
  using Base = Container<Poly<T, N>, T, N, true>;

 public:
  using Base::Base;

  using typename Base::raw_value_type;
  using typename Base::value_type;

  using Base::begin;
  using Base::end;
  using Base::operator[];

  Poly& operator+=(const Poly& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < other.size(); ++i) {
      (*this)[i] = static_cast<value_type>((*this)[i]) +
                   static_cast<value_type>(other[i]);
    }
    return *this;
  }

  Poly& operator-=(const Poly& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < this->size(); ++i) {
      (*this)[i] = static_cast<value_type>((*this)[i]) -
                   static_cast<value_type>(other[i]);
    }
    return *this;
  }

  template <typename Expr>
    requires requires(Expr& ep) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly(const Expr& ep) {
    std::fill(begin(), end(), typename T::raw_value_type{});
    accumulate_expr(
        ep,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        });
  }

  template <typename Expr>
    requires requires(Expr& ep) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly& operator=(const Expr& ep) {
    accumulate_expr(
        ep,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        });
    return *this;
  }

  template <typename Expr>
    requires requires(Expr& ep) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly& operator+=(const Expr& ep) {
    accumulate_expr(
        ep,
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) + static_cast<value_type>(y);
        },
        [](auto x, const auto& y) {
          x = static_cast<value_type>(x) - static_cast<value_type>(y);
        });
    return *this;
  }

  template <typename Expr>
    requires requires(Expr& ep) {
      { std::derived_from<Expr, expr_tag> };
    }
  Poly& operator-=(const Expr& ep) {
    accumulate_expr(
        ep,
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
  void accumulate_expr(const Expr& ep, AddOp add_op, SubOp sub_op) {
    accumulate_impl<Expr>::apply(*this, ep, add_op, sub_op);
  }
};

template <typename T, uint32_t N>
inline Poly<T, N> operator+(Poly<T, N> lhs, const Poly<T, N>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs += rhs;
}

template <typename T, uint32_t N, typename Expr>
inline Poly<T, N> operator+(Poly<T, N> lhs, const Expr& rhs) {
  return lhs += rhs;
}

template <typename T, uint32_t N>
inline Poly<T, N> operator-(Poly<T, N> lhs, const Poly<T, N>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs -= rhs;
}

template <typename T, uint32_t N, typename Expr>
inline Poly<T, N> operator-(Poly<T, N> lhs, const Expr& rhs) {
  return lhs -= rhs;
}

// If operator+(Proxy lhs, const Proxy<T>& rhs) is used,
// lhs is copied together with its reference state,
// which results in unintended behavior.
template <typename T, uint32_t N>
inline Poly<T, N>::value_type operator+(const typename Poly<T, N>::Proxy& lhs,
                                        const typename Poly<T, N>::Proxy& rhs) {
  return static_cast<Poly<T, N>::raw_value_type>(lhs) +
         static_cast<Poly<T, N>::raw_value_type>(rhs);
}

// If operator-(Proxy lhs, const Proxy<T>& rhs) is used,
// lhs is copied together with its reference state,
// which results in unintended behavior.
template <typename T, uint32_t N>
inline Poly<T, N>::value_type operator-(const typename Poly<T, N>::Proxy& lhs,
                                        const typename Poly<T, N>::Proxy& rhs) {
  return static_cast<Poly<T, N>::raw_value_type>(lhs) -
         static_cast<Poly<T, N>::raw_value_type>(rhs);
}

#endif  // ALGEBRA_POLY_HPP
