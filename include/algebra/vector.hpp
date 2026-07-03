// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_VECTOR_HPP
#define ALGEBRA_VECTOR_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>

#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"

#include "detail/proxy.hpp"

template <typename T, typename = void>
struct vector_traits {
  using value_type = T;
  using raw_value_type = T;
  static constexpr bool use_proxy = false;
};

template <primitive T>
struct vector_traits<T, std::void_t<typename T::raw_value_type>> {
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;
  static constexpr bool use_proxy = !std::same_as<T, raw_value_type>;
};

template <typename T, uint32_t n>
class Vector {
 public:
  using traits = vector_traits<T>;

  using value_type = typename traits::value_type;
  using raw_value_type = typename traits::raw_value_type;

  using iterator = raw_value_type*;
  using const_iterator = const raw_value_type*;

  Vector() = default;

  Vector(const Vector& other) {
    std::copy_n(other.data_.get(), n, data_.get());
  }

  Vector& operator=(const Vector& other) {
    if (this == &other) return *this;
    std::copy_n(other.data_.get(), n, data_.get());
    return *this;
  }

  Vector(Vector&&) noexcept = default;
  Vector& operator=(Vector&&) noexcept = default;

  template <typename F>
    requires requires(F& f, std::size_t i) {
      { std::invoke(f, i) } -> explicitly_convertible_to<T>;
    }
  Vector(F&& f) {
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
  Vector(F&& f) {
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f));
    });
  }

  decltype(auto) operator[](size_t idx) noexcept {
    if constexpr (traits::use_proxy)
      return Proxy<Vector>(data_.get() + idx);
    else
      return static_cast<value_type&>(data_[idx]);
  }
  decltype(auto) operator[](size_t idx) const noexcept {
    if constexpr (traits::use_proxy)
      return Proxy<Vector>(data_.get() + idx);
    else
      return static_cast<value_type&>(data_[idx]);
  }

  constexpr iterator begin() noexcept { return data_.get(); }
  constexpr iterator end() noexcept { return data_.get() + n; }

  constexpr const_iterator begin() const noexcept { return data_.get(); }
  constexpr const_iterator end() const noexcept { return data_.get() + n; }

  T* data() { return data_.get(); }
  const T* data() const { return data_.get(); }

  constexpr size_t size() const { return n; }

  friend std::ostream& operator<<(std::ostream& os, const Vector& vec) {
    os << "Vec(";
    for (size_t i = 0; i < vec.size(); ++i) {
      os << vec[i];
      if (i + 1 < vec.size()) {
        os << ", ";
      }
    }
    os << ")";
    return os;
  }

 private:
  std::unique_ptr<raw_value_type[]> data_ =
      std::make_unique<raw_value_type[]>(n);
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