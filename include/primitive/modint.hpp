// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_MODINT_HPP
#define PRIMITIVE_MODINT_HPP

#include <cstdint>
#include <iostream>

#include "primitive/uint.hpp"

template <uint32_t P>
class ModInt {
 public:
  static constexpr uint32_t mod = P;
  using raw_value_type = uint32_t;

  constexpr ModInt() noexcept = default;

  template <std::integral Raw>
    requires std::convertible_to<Raw, raw_value_type>
  constexpr explicit ModInt(Raw raw = 0) noexcept {
    if constexpr (mod == 0) {
      value_ = static_cast<raw_value_type>(raw);
    } else {
      value_ = static_cast<raw_value_type>(raw) % mod;
    }
  }

  static constexpr raw_value_type raw_min() { return 0; }
  static constexpr raw_value_type raw_max() { return P - 1; }

  constexpr explicit operator raw_value_type() const noexcept {
    return value_;
  }
  constexpr raw_value_type value() const noexcept { return value_; }

  constexpr ModInt& operator+=(const ModInt& rhs) noexcept {
    if constexpr (mod == 0) {
      value_ += rhs.value_;
    } else {
      static_assert(mod <= std::numeric_limits<raw_value_type>::max() << 1,
                    "mod is too large for addition");
      value_ += rhs.value_;
      raw_value_type mask = -static_cast<raw_value_type>(value_ >= mod);
      value_ -= (mod & mask);
    }
    return *this;
  }

  // when a < b,
  // compute a - b = (a - b) + P
  constexpr ModInt& operator-=(const ModInt& rhs) noexcept {
    if constexpr (mod == 0) {
      value_ -= rhs.value_;
    } else {
      raw_value_type mask = -static_cast<raw_value_type>(value_ < rhs.value_);
      value_ -= rhs.value_;
      value_ += (mod & mask);
    }
    return *this;
  }

  constexpr bool operator==(const ModInt& other) const noexcept {
    return value_ == other.value_;
  }

  friend std::ostream& operator<<(std::ostream& os, const ModInt& m) {
    return os << m.value_;
  }

 private:
  raw_value_type value_;
};

template <uint32_t P>
inline constexpr ModInt<P> operator+(ModInt<P> lhs,
                                     const ModInt<P>& rhs) noexcept {
  return lhs += rhs;
}

template <uint32_t P>
inline constexpr ModInt<P> operator-(ModInt<P> lhs,
                                     const ModInt<P>& rhs) noexcept {
  return lhs -= rhs;
}

template <uint32_t P>
inline constexpr ModInt<P> operator-(const ModInt<P>& x) noexcept {
  return ModInt<P>(0) - x;
}

template <uint32_t P>
inline constexpr ModInt<P> operator*(UInt lhs, const ModInt<P>& rhs) noexcept {
  return ModInt<P>(static_cast<ModInt<P>::raw_value_type>(lhs) * rhs.value());
}

#endif  // PRIMITIVE_MODINT_HPP