#ifndef POLY_HPP
#define POLY_HPP

#include <algorithm>
#include <cassert>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
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

  class Proxy;

  Poly() = default;
  explicit Poly(size_t N)
      : coeffs_(std::make_unique<raw_value_type[]>(N)), size_(N) {
    std::fill_n(coeffs_.get(), size_, raw_value_type{});
  }

  template <typename F>
    requires requires(F& f) {
      { std::invoke(f) } -> explicitly_convertible_to<T>;
    }
  explicit Poly(size_t N, F&& f) : Poly(N) {
    for (size_t i = 0; i < size_; ++i) {
      coeffs_[i] = static_cast<raw_value_type>(std::invoke(f));
    }
  }

  explicit Poly(const raw_value_type* ptr, size_t N) : Poly(N) {
    std::copy(ptr, ptr + size_, coeffs_.get());
  }

  explicit Poly(std::unique_ptr<raw_value_type[]>&& ptr, size_t N) noexcept
      : coeffs_(std::move(ptr)), size_(N) {}

  Poly(std::initializer_list<T> init) : Poly(init.size()) {
    size_t i = 0;
    for (const auto& v : init) {
      (*this)[i++] = v;
    }
  }

  Poly(const Poly& other) : Poly(other.size()) {
    std::copy(other.coeffs_.get(), other.coeffs_.get() + other.size_,
              coeffs_.get());
  }

  Poly& operator=(const Poly& other) {
    if (this == &other) return *this;

    this->size_ = other.size_;
    this->coeffs_ = std::make_unique<raw_value_type[]>(other.size_);

    std::copy(other.coeffs_.get(), other.coeffs_.get() + other.size_,
              coeffs_.get());

    return *this;
  }

  Poly(Poly&& other) noexcept = default;
  Poly& operator=(Poly&& other) noexcept = default;

  Proxy operator[](size_t idx) noexcept { return Proxy(coeffs_.get() + idx); }
  value_type operator[](size_t idx) const noexcept {
    return static_cast<value_type>(coeffs_[idx]);
  }

  const raw_value_type* data() const noexcept { return coeffs_.get(); }

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
    for (size_t i = 0; i < this->size(); ++i) {
      if (static_cast<value_type>(coeffs_[i]) != other[i]) {
        return false;
      }
    }
    return true;
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
  std::unique_ptr<raw_value_type[]> coeffs_;
  size_t size_;
};

template <typename To, typename From>
  requires explicitly_convertible_to<To, From>
inline Poly<To> convert_to(const Poly<From>& poly) {
  Poly<To> dst(poly.size());
  for (size_t i = 0; i < poly.size(); ++i) {
    dst[i] = static_cast<To>(static_cast<From>(poly[i]));
  }
  return dst;
}

template <typename To, typename From>
  requires explicitly_convertible_to<To, From>
inline Poly<To> convert_to(Poly<From>&& poly) {
  Poly<To> dst(poly.size());
  for (size_t i = 0; i < poly.size(); ++i) {
    dst[i] = static_cast<To>(static_cast<From>(poly[i]));
  }
  return dst;
}

template <typename To, typename From>
  requires interpretable_to<To, From>
inline Poly<To> interpret_as(const Poly<From>& src) {
  std::unique_ptr<typename To::raw_value_type[]> ptr =
      std::make_unique<typename From::raw_value_type[]>(src.size());
  std::copy(src.data(), src.data() + src.size(), ptr.get());
  return Poly<To>(std::move(ptr), src.size());
}

template <typename To, typename From>
  requires interpretable_to<To, From>
inline Poly<To> interpret_as(Poly<From>&& src) {
  size_t N = src.size_;
  return Poly<To>(std::move(src.coeffs_), N);
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
