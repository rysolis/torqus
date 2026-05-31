#ifndef POLY_HPP
#define POLY_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

#include "primitive/concept/convertible.hpp"
#include "primitive/concept/interpretable.hpp"
#include "primitive/concept/primitive.hpp"

template <typename T>
class Poly {
 public:
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;

  using iterator = raw_value_type*;
  using const_iterator = const raw_value_type*;

  // rule of five
  Poly() = default;

  Poly(const Poly& other) : Poly(other.begin(), other.end()) {}
  Poly& operator=(const Poly& other) {
    if (this == &other) return *this;
    Poly tmp(other.begin(), other.end());
    std::swap(*this, tmp);
    return *this;
  }

  Poly(Poly&& other) noexcept = default;
  Poly& operator=(Poly&& other) noexcept = default;
  // end of rule of five

  explicit Poly(size_t N)
      : coeffs_(std::make_unique<raw_value_type[]>(N)), size_(N) {
    std::fill_n(coeffs_.get(), size_, raw_value_type{});
  }

  Poly(std::initializer_list<T> init) : Poly(init.size()) {
    std::ranges::transform(init, begin(), [](const T& v) {
      return static_cast<raw_value_type>(v);
    });
  }

  template <typename F>
    requires requires(F& f) {
      { std::invoke(f) } -> explicitly_convertible_to<T>;
    }
  Poly(size_t N, F&& f) : Poly(N) {
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f));
    });
  }

  Poly(const raw_value_type* ptr, size_t N) : Poly(ptr, ptr + N) {}

  class Proxy;

  Proxy operator[](size_t idx) noexcept { return Proxy(coeffs_.get() + idx); }
  value_type operator[](size_t idx) const noexcept {
    return static_cast<value_type>(coeffs_[idx]);
  }

  constexpr iterator begin() noexcept { return coeffs_.get(); }
  constexpr iterator end() noexcept { return coeffs_.get() + size_; }

  constexpr const_iterator begin() const noexcept { return coeffs_.get(); }
  constexpr const_iterator end() const noexcept {
    return coeffs_.get() + size_;
  }

  constexpr raw_value_type* data() noexcept { return coeffs_.get(); }
  constexpr raw_value_type* data() const noexcept { return coeffs_.get(); }

  constexpr size_t size() const noexcept { return size_; }

  Poly& operator+=(const Poly& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < other.size(); ++i) {
      (*this)[i] += other[i];
    }
    return *this;
  }

  Poly& operator-=(const Poly& other) {
    assert(other.size() == this->size());

    for (size_t i = 0; i < this->size(); ++i) {
      (*this)[i] -= other[i];
    }
    return *this;
  }

  constexpr bool operator==(const Poly& other) const noexcept {
    if (other.size() != this->size()) {
      return false;
    }
    return std::ranges::equal(*this, other);
  }

  template <typename To, typename From>
    requires explicitly_convertible_to<To, From>
  friend Poly<To> convert_to(const Poly<From>& poly);

  template <typename To, typename From>
    requires explicitly_convertible_to<To, From>
  friend Poly<To> convert_to(Poly<From>&& poly);

  template <typename To, typename From>
    requires interpretable_to<To, From>
  friend Poly<To> interpret_as(const Poly<From>&);

  template <typename To, typename From>
    requires interpretable_to<To, From>
  friend Poly<To> interpret_as(Poly<From>&&);

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
  template <std::forward_iterator It>
  Poly(It first, It last)
      : Poly(static_cast<size_t>(std::distance(first, last))) {
    std::copy(first, last, begin());
  }

  // This constructor is used by interpret_as to take ownership of the buffer
  Poly(std::unique_ptr<raw_value_type[]>&& ptr, size_t N) noexcept
      : coeffs_(std::move(ptr)), size_(N) {}

  std::unique_ptr<raw_value_type[]> coeffs_;
  size_t size_;
};

template <typename To, typename From>
  requires explicitly_convertible_to<To, From>
inline Poly<To> convert_to(const Poly<From>& src) {
  Poly<To> dst(src.size());
  std::ranges::transform(src.begin(), src.end(), dst.begin(),
                         [](const From::raw_value_type& v) {
                           return static_cast<To::raw_value_type>(
                               static_cast<To>(static_cast<From>(v)));
                         });
  return dst;
}

template <typename To, typename From>
  requires explicitly_convertible_to<To, From>
inline Poly<To> convert_to(Poly<From>&& src) {
  Poly<To> dst(src.size());
  std::ranges::transform(src.begin(), src.end(), dst.begin(),
                         [](const From::raw_value_type& v) {
                           return static_cast<To::raw_value_type>(
                               static_cast<To>(static_cast<From>(v)));
                         });
  return dst;
}

template <typename To, typename From>
  requires interpretable_to<To, From>
inline Poly<To> interpret_as(const Poly<From>& src) {
  return Poly<To>(src.data(), src.size());
}

template <typename To, typename From>
  requires interpretable_to<To, From>
inline Poly<To> interpret_as(Poly<From>&& src) {
  return Poly<To>(std::move(src.coeffs_), src.size());
}

template <typename T>
inline Poly<T> operator+(Poly<T> lhs, const Poly<T>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs += rhs;
}

template <typename T>
inline Poly<T> operator-(Poly<T> lhs, const Poly<T>& rhs) {
  assert(lhs.size() == rhs.size());
  return lhs -= rhs;
}

#include "detail/proxy.hpp"

// If operator+(Proxy lhs, const Proxy<T>& rhs) is used,
// lhs is copied together with its reference state,
// which results in unintended behavior.
template <typename T>
inline Poly<T>::Proxy::T operator+(const typename Poly<T>::Proxy& lhs,
                                   const typename Poly<T>::Proxy& rhs) {
  return static_cast<Poly<T>::Proxy::T>(lhs) +
         static_cast<Poly<T>::Proxy::T>(rhs);
}

// If operator-(Proxy lhs, const Proxy<T>& rhs) is used,
// lhs is copied together with its reference state,
// which results in unintended behavior.
template <typename T>
inline Poly<T>::Proxy::T operator-(const typename Poly<T>::Proxy& lhs,
                                   const typename Poly<T>::Proxy& rhs) {
  return static_cast<Poly<T>::Proxy::T>(lhs) -
         static_cast<Poly<T>::Proxy::T>(rhs);
}

#endif
