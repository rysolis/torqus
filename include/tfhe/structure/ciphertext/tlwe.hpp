// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TLWE_HPP
#define TFHE_TLWE_HPP

#include <cstdint>
#include <iostream>
#include <memory>

#include "primitive/concept/torus.hpp"

#include "algebra/vector.hpp"

template <torus_type Torus, uint32_t n>
class TLWE {
 public:
  TLWE() = default;

  TLWE(const TLWE& other) : b_(other.b()) {
    std::copy(other.a().begin(), other.a().end(), a_.begin());
  }

  TLWE& operator=(const TLWE& other) {
    if (this == &other) return *this;

    std::copy(other.a().begin(), other.a().end(), a_.begin());
    b_ = other.b_;
    return *this;
  }

  template <typename F>
    requires requires(F& f, std::size_t i) {
      { std::invoke(f, i) } -> explicitly_convertible_to<Torus>;
    }
  TLWE(F&& f) : a_(std::forward<F>(f)) {}

  template <typename F>
    requires(
        requires(F& f, std::size_t i) {
          { std::invoke(f, i) } -> explicitly_convertible_to<Torus>;
        } &&
        requires(F& f) {
          { std::invoke(f) } -> explicitly_convertible_to<Torus>;
        })
  TLWE(F&& f) : a_(std::forward<F>(f)) {}

  Vector<Torus, n>& a() { return a_; }
  const Vector<Torus, n>& a() const { return a_; }
  Torus& b() { return b_; }
  const Torus& b() const { return b_; }

  constexpr uint32_t dimension() const noexcept { return n; }

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

#endif  // TFHE_TLWE_HPP