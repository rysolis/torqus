// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_UINT_HPP
#define PRIMITIVE_UINT_HPP

#include <cassert>
#include <cstdint>
#include <iostream>
#include <limits>

class UInt {
 public:
  using raw_value_type = uint32_t;

  explicit UInt() noexcept = default;

  template <std::integral Raw>
    requires std::convertible_to<Raw, raw_value_type>
  constexpr explicit UInt(Raw v = 0) noexcept
      : v_(static_cast<raw_value_type>(v)) {}

  static constexpr raw_value_type raw_min() { return 0; }
  static constexpr raw_value_type raw_max() {
    return std::numeric_limits<raw_value_type>::max();
  }

  constexpr explicit operator raw_value_type() const noexcept { return v_; }

  inline constexpr UInt& operator+=(const UInt& rhs) {
    if (v_ > std::numeric_limits<raw_value_type>::max() - rhs.v_) {
      throw std::overflow_error("UInt addition overflow");
    }
    v_ += rhs.v_;
    return *this;
  }

  inline constexpr UInt& operator-=(const UInt& rhs) {
    if (v_ < rhs.v_) {
      throw std::underflow_error("UInt subtraction underflow");
    }
    v_ -= rhs.v_;
    return *this;
  }

  constexpr bool operator==(const UInt& other) const noexcept {
    return v_ == other.v_;
  }

  friend std::ostream& operator<<(std::ostream& os, const UInt& u) {
    return os << static_cast<UInt::raw_value_type>(u);
  }

 private:
  raw_value_type v_;
};

inline constexpr UInt operator+(UInt lhs, const UInt& rhs) {
  return lhs += rhs;
}

inline constexpr UInt operator-(UInt lhs, const UInt& rhs) {
  return lhs -= rhs;
}

#endif  // PRIMITIVE_UINT_HPP