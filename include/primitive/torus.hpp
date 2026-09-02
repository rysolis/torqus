// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_TORUS_HPP
#define PRIMITIVE_TORUS_HPP

#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>
#include <limits>

#include "primitive/uint.hpp"
#include "primitive/word.hpp"

/**
 * \brief Base class for torus types.
 *
 * Defines common operations for Torus and ModTorus
 * using the CRTP mixin pattern.
 */
template <typename Derived>
class TorusBase {
 public:
  friend Derived;
  constexpr const Derived& self() const {
    return static_cast<const Derived&>(*this);
  }

  constexpr Derived& self() { return static_cast<Derived&>(*this); }

  friend std::ostream& operator<<(std::ostream& os, const TorusBase& t) {
    return os << t.self().value();
  }

 private:
  TorusBase() = default;
};

template <uint32_t QBit, typename Word = torqus_default_word_t>
class ModTorus;

namespace dbl {

/**
 * \brief Represents the discrete torus $(1/q)\mathbb{R}/\mathbb{Z}$.
 *
 * The canonical representative is taken in the interval [0, 1).
 */
class Torus : public TorusBase<Torus> {
 public:
  using value_type = Torus;
  using raw_value_type = double;

  explicit Torus() noexcept = default;

  template <std::floating_point Raw>
    requires std::convertible_to<Raw, raw_value_type>
  constexpr explicit Torus(Raw raw = 0.0) noexcept
      : value_(static_cast<raw_value_type>(raw) -
               std::floor(static_cast<raw_value_type>(raw))) {}

  static constexpr raw_value_type raw_min() { return 0.0; }
  static constexpr raw_value_type raw_max() { return 1.0; }

  template <uint32_t QBit, typename Word>
  constexpr explicit operator ModTorus<QBit, Word>() const noexcept;

  constexpr explicit operator raw_value_type() const noexcept { return value_; }
  constexpr raw_value_type value() const noexcept { return value_; }

  bool operator==(const Torus& other) const noexcept;

  constexpr Torus& operator+=(const Torus& other) noexcept {
    value_ += other.value_;
    value_ -= std::floor(value_);
    return *this;
  }

  constexpr Torus& operator-=(const Torus& other) noexcept {
    value_ -= other.value_;
    value_ -= std::floor(value_);
    return *this;
  }

 private:
  raw_value_type value_;
};

}  // namespace dbl

/**
 * \brief Represents the integer torus $\hat{\mathbb{T}}$.
 *
 * The following correspondence holds:
 *
 * $\hat{\mathbb{T}} \ni c \sim q \cdot t \in (1/q)\mathbb{R}/\mathbb{Z}$.
 */
template <uint32_t QBit, typename Word>
class ModTorus : public TorusBase<ModTorus<QBit, Word>> {
 public:
  using value_type = ModTorus;
  using raw_value_type = Word;
  static constexpr uint32_t qbit = QBit;

  static_assert(QBit <= std::numeric_limits<raw_value_type>::digits,
                "QBit must fit within Word's bit width");

  explicit ModTorus() noexcept = default;

  template <std::integral Raw>
  constexpr explicit ModTorus(Raw raw = 0) noexcept
      : value_(static_cast<raw_value_type>(raw) & mask()) {}

  // Constructs from a value `raw` given as a numerator over `resolution`
  // (i.e. representing raw/resolution mod 1). `resolution` may be any
  // positive integer, not just a power of two -- a power-of-two
  // resolution divides evenly into 2^qbit and so is represented exactly;
  // any other resolution is rounded down to the nearest representable
  // qbit-bit value, same as any other torus quantization.
  //
  // Computes floor(raw * 2^qbit / resolution) mod 2^qbit via binary long
  // division (one quotient bit per iteration) rather than forming
  // raw * 2^qbit directly, since that product generally doesn't fit in
  // raw_value_type once qbit is large -- this way every intermediate
  // stays within raw_value_type's own width.
  template <std::integral Raw>
  constexpr ModTorus(Raw raw, Raw resolution) noexcept {
    assert(resolution > 0);
    raw_value_type divisor = static_cast<raw_value_type>(resolution);
    raw_value_type remainder = static_cast<raw_value_type>(raw) % divisor;
    raw_value_type quotient = 0;
    for (uint32_t i = 0; i < qbit; ++i) {
      // remainder is always < divisor going in, so doubling it can only
      // ever overflow raw_value_type by the one bit captured here.
      bool overflowed =
          (remainder >> (std::numeric_limits<raw_value_type>::digits - 1)) != 0;
      remainder = static_cast<raw_value_type>(remainder << 1);
      quotient = static_cast<raw_value_type>(quotient << 1);
      if (overflowed || remainder >= divisor) {
        remainder = static_cast<raw_value_type>(remainder - divisor);
        quotient |= 1;
      }
    }
    value_ = quotient & mask();
  }

  static constexpr raw_value_type raw_min() { return 0; }
  static constexpr raw_value_type raw_max() {
    return static_cast<raw_value_type>(-1) & mask();
  }

  constexpr explicit operator dbl::Torus() const noexcept;
  constexpr explicit operator double() const noexcept {
    return static_cast<double>(value_) / std::pow(2., qbit);
  }

  constexpr explicit operator raw_value_type() const noexcept { return value_; }
  constexpr raw_value_type value() const noexcept { return value_; }

  bool operator==(const ModTorus& other) const noexcept {
    return this->self().value() == other.value();
  }

  constexpr ModTorus& operator+=(const ModTorus& other) noexcept {
    value_ += other.value_;
    value_ &= mask();
    return *this;
  }

  constexpr ModTorus& operator-=(const ModTorus& other) noexcept {
    value_ -= other.value_;  // Since Q is a power of two, using an unsigned
                             // integer for value_ is safe.
    value_ &= mask();
    return *this;
  }

  static constexpr raw_value_type mask() noexcept {
    return std::numeric_limits<raw_value_type>::max() >>
           (std::numeric_limits<raw_value_type>::digits - qbit);
  }

 private:
  raw_value_type value_;
};

template <uint32_t QBit, typename Word>
inline constexpr dbl::Torus::operator ModTorus<QBit, Word>() const noexcept {
  return ModTorus<QBit, Word>(
      static_cast<typename ModTorus<QBit, Word>::raw_value_type>(
          std::pow(2., ModTorus<QBit, Word>::qbit) * this->value()));
}

template <uint32_t QBit, typename Word>
inline constexpr ModTorus<QBit, Word>::operator dbl::Torus() const noexcept {
  return dbl::Torus(static_cast<dbl::Torus::raw_value_type>(this->value()) /
                    std::pow(2., ModTorus<QBit, Word>::qbit));
}

inline constexpr dbl::Torus operator+(dbl::Torus lhs,
                                      const dbl::Torus& rhs) noexcept {
  return lhs += rhs;
}

inline constexpr dbl::Torus operator-(dbl::Torus lhs,
                                      const dbl::Torus& rhs) noexcept {
  return lhs -= rhs;
}

inline constexpr dbl::Torus operator*(UInt lhs,
                                      const dbl::Torus& rhs) noexcept {
  return dbl::Torus(
      static_cast<double>(static_cast<UInt::raw_value_type>(lhs)) *
      rhs.value());
}

template <uint32_t QBit, typename Word>
inline constexpr ModTorus<QBit, Word> operator*(
    UInt lhs, const ModTorus<QBit, Word>& rhs) noexcept {
  return ModTorus<QBit, Word>(
      static_cast<typename ModTorus<QBit, Word>::raw_value_type>(lhs) *
      rhs.value());
}

template <uint32_t QBit, typename Word>
inline constexpr ModTorus<QBit, Word> operator+(
    ModTorus<QBit, Word> lhs, const ModTorus<QBit, Word>& rhs) noexcept {
  return lhs += rhs;
}

template <uint32_t QBit, typename Word>
inline constexpr ModTorus<QBit, Word> operator-(
    ModTorus<QBit, Word> lhs, const ModTorus<QBit, Word>& rhs) noexcept {
  return lhs -= rhs;
}

template <uint32_t QBit, typename Word>
inline constexpr ModTorus<QBit, Word> operator-(
    const ModTorus<QBit, Word>& x) noexcept {
  return ModTorus<QBit, Word>{} - x;
}

inline constexpr double infinity_norm(const dbl::Torus& torus) {
  double tmp = static_cast<double>(torus);
  return std::min(tmp, 1 - tmp);
}

template <uint32_t QBit, typename Word>
inline constexpr double infinity_norm(const ModTorus<QBit, Word>& mod_torus) {
  typename dbl::Torus::raw_value_type tmp =
      dbl::Torus::raw_value_type(static_cast<dbl::Torus>(mod_torus));
  return std::min(tmp, 1 - tmp);
}

// Torus::operator== depends on infinity_norm, which is defined in
// utility.hpp, so we define it here to avoid circular dependency
inline bool dbl::Torus::operator==(const dbl::Torus& other) const noexcept {
  const double eps = 1e-9;
  double norm = infinity_norm((*this) - other);
  return norm < eps;
}

#endif  // PRIMITIVE_TORUS_HPP