// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TLWE_HPP
#define TFHE_TLWE_HPP

#include <cstdint>
#include <iostream>
#include <memory>

#include "primitive/concept/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/vector.hpp"

template <torus_concept Torus, uint32_t n>
class TLWE {
 public:
  TLWE() = default;

  TLWE(const TLWE& other) : a_(other.a()), b_(other.b()) {}

  TLWE(TLWE&&) = default;
  TLWE& operator=(TLWE&&) = default;

  template <typename F>
    requires requires(F& f, std::size_t i) {
      { std::invoke(f, i) } -> explicitly_convertible_to_concept<Torus>;
    }
  TLWE(F&& f) : a_(std::forward<F>(f)) {}

  template <typename F>
    requires(
        requires(F& f, std::size_t i) {
          { std::invoke(f, i) } -> explicitly_convertible_to_concept<Torus>;
        } &&
        requires(F& f) {
          { std::invoke(f) } -> explicitly_convertible_to_concept<Torus>;
        })
  TLWE(F&& f) : a_(std::forward<F>(f)) {}

  Vector<Torus, n>& a() { return a_; }
  const Vector<Torus, n>& a() const { return a_; }
  Torus& b() { return b_; }
  const Torus& b() const { return b_; }

  TLWE& operator+=(const TLWE& other) {
    a_ += other.a_;
    b_ += other.b_;
    return *this;
  }

  TLWE& operator-=(const TLWE& other) {
    a_ -= other.a_;
    b_ -= other.b_;
    return *this;
  }

  constexpr uint32_t dimension() const noexcept { return n; }

  const void* identity() const noexcept {
    return static_cast<const void*>(a_.data());
  }

  friend std::ostream& operator<<(std::ostream& os, const TLWE& tlwe) {
    os << "TLWE(a: ";
    for (size_t i = 0; i < n; ++i) {
      os << tlwe.a_[i] << " ";
    }
    os << ", b: " << tlwe.b_ << ")";
    return os;
  }

 private:
  Vector<Torus, n> a_;
  Torus b_{};
};

template <torus_concept Torus, uint32_t n>
inline constexpr TLWE<Torus, n> operator*(UInt lhs, const TLWE<Torus, n>& rhs) {
  TLWE<Torus, n> res;
  res.b() = lhs * Torus(rhs.b());
  for (size_t i = 0; i < rhs.dimension(); ++i) {
    res.a()[i] = lhs * Torus(rhs.a()[i]);
  }
  return res;
}

#endif  // TFHE_TLWE_HPP