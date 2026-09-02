// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_UINT_HPP
#define PRIMITIVE_UINT_HPP

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

#include "primitive/word.hpp"

class UInt {
 public:
  using raw_value_type = torqus_default_word_t;

  explicit UInt() noexcept = default;

  template <std::integral Raw>
    requires std::convertible_to<Raw, raw_value_type>
  constexpr explicit UInt(Raw raw = 0) noexcept
      : value_(static_cast<raw_value_type>(raw)) {}

  static constexpr raw_value_type raw_min() { return 0; }
  static constexpr raw_value_type raw_max() {
    return std::numeric_limits<raw_value_type>::max();
  }

  constexpr explicit operator raw_value_type() const noexcept { return value_; }

  inline constexpr UInt& operator+=(const UInt& rhs) {
    if (value_ > std::numeric_limits<raw_value_type>::max() - rhs.value_) {
      throw std::overflow_error("UInt addition overflow");
    }
    value_ += rhs.value_;
    return *this;
  }

  inline constexpr UInt& operator-=(const UInt& rhs) {
    if (value_ < rhs.value_) {
      throw std::underflow_error("UInt subtraction underflow");
    }
    value_ -= rhs.value_;
    return *this;
  }

  constexpr bool operator==(const UInt& other) const noexcept {
    return value_ == other.value_;
  }

  friend std::ostream& operator<<(std::ostream& os, const UInt& u) {
    return os << static_cast<UInt::raw_value_type>(u);
  }

 private:
  raw_value_type value_;
};

inline constexpr UInt operator+(UInt lhs, const UInt& rhs) {
  return lhs += rhs;
}

inline constexpr UInt operator-(UInt lhs, const UInt& rhs) {
  return lhs -= rhs;
}

#endif  // PRIMITIVE_UINT_HPP