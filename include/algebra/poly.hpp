// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_POLY_HPP
#define ALGEBRA_POLY_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <ranges>
#include <type_traits>

#include "primitive/concept/convertible.hpp"

#include "detail/proxy.hpp"

template <typename T, uint32_t N>
class Poly {
 public:
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;

  using iterator = raw_value_type*;
  using const_iterator = const raw_value_type*;

  Poly() = default;

  // rule of five
  Poly(const Poly& other) : Poly(other.begin(), other.end()) {}
  Poly& operator=(const Poly& other) {
    if (this == &other) return *this;
    Poly tmp(other);
    std::swap(*this, tmp);
    return *this;
  }

  Poly(Poly&& other) noexcept = default;
  Poly& operator=(Poly&& other) noexcept = default;

  ~Poly() = default;
  // end of rule of five

  Poly(std::initializer_list<T> init) {
    std::ranges::transform(init, begin(), [](const T& v) {
      return static_cast<raw_value_type>(v);
    });
  }

  template <typename F>
    requires requires(F& f, std::size_t i) {
      { std::invoke(f, i) } -> explicitly_convertible_to<T>;
    }
  Poly(F&& f) {
    std::size_t i = 0;
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f, i++));
    });
  }

  template <typename F>
    requires(
        !requires(F& f, std::size_t i) {
          { std::invoke(f, i) } -> explicitly_convertible_to<T>;
        } &&
        requires(F& f) {
          { std::invoke(f) } -> explicitly_convertible_to<T>;
        })
  Poly(F&& f) {
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f));
    });
  }

  template <std::forward_iterator It>
  Poly(It first, It last) {
    std::copy(first, last, begin());
  }

  Poly(const raw_value_type* ptr) : Poly(ptr, ptr + N) {}

  Proxy<Poly> operator[](size_t idx) noexcept {
    assert(idx < N);
    return Proxy<Poly>(coeffs_.get() + idx);
  }
  value_type operator[](size_t idx) const noexcept {
    assert(idx < N);
    return static_cast<value_type>(coeffs_[idx]);
  }

  constexpr iterator begin() noexcept { return coeffs_.get(); }
  constexpr iterator end() noexcept { return coeffs_.get() + N; }

  constexpr const_iterator begin() const noexcept { return coeffs_.get(); }
  constexpr const_iterator end() const noexcept { return coeffs_.get() + N; }

  constexpr raw_value_type* data() noexcept { return coeffs_.get(); }
  constexpr const raw_value_type* data() const noexcept {
    return coeffs_.get();
  }

  static constexpr size_t size() { return N; }

  Poly& operator+=(const Poly& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < other.size(); ++i) {
      (*this)[i] = static_cast<value_type>((*this)[i]) + other[i];
    }
    return *this;
  }

  Poly& operator-=(const Poly& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < this->size(); ++i) {
      (*this)[i] = static_cast<value_type>((*this)[i]) - other[i];
    }
    return *this;
  }

  template <typename Expr>
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

  [[nodiscard]]
  raw_value_type* release() {
    return coeffs_.release();
  }

  // This constructor is used by interpret_as to take ownership of the buffer
  Poly(raw_value_type* ptr) noexcept : coeffs_(ptr) {}

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
  void accumulate_expr(const Expr& ep, AddOp add_op, SubOp sub_op);

  std::unique_ptr<raw_value_type[]> coeffs_ =
      std::make_unique<raw_value_type[]>(N);
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
