// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_MODINT_HPP
#define PRIMITIVE_MODINT_HPP

#include <cstdint>
#include <iostream>
#include <limits>

#include "primitive/uint.hpp"
#include "primitive/word.hpp"

// P is uint64_t (wider than Word can be, e.g. when Word is uint32_t) so
// that a caller wrapping a full-width Torus value (see
// tfhe/operation/bootstrap/gate_bootstrap.hpp's ModInt<Q>) can name a
// modulus up to Word's full range; mod below narrows it back down to
// Word once, at a point where P is a compile-time constant, so every
// other member keeps operating in plain Word arithmetic exactly as
// before.
template <uint64_t P, typename Word = torqus_default_word_t>
class ModInt {
 public:
  using raw_value_type = Word;

  static_assert(P == 0 || P <= std::numeric_limits<raw_value_type>::max(),
                "P must fit within Word, or be 0 (meaning natural "
                "wraparound at Word's own width)");

  static constexpr raw_value_type mod = static_cast<raw_value_type>(P);

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
  static constexpr raw_value_type raw_max() { return mod - 1; }

  constexpr explicit operator raw_value_type() const noexcept { return value_; }
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

template <uint64_t P, typename Word>
inline constexpr ModInt<P, Word> operator+(
    ModInt<P, Word> lhs, const ModInt<P, Word>& rhs) noexcept {
  return lhs += rhs;
}

template <uint64_t P, typename Word>
inline constexpr ModInt<P, Word> operator-(
    ModInt<P, Word> lhs, const ModInt<P, Word>& rhs) noexcept {
  return lhs -= rhs;
}

template <uint64_t P, typename Word>
inline constexpr ModInt<P, Word> operator-(const ModInt<P, Word>& x) noexcept {
  return ModInt<P, Word>(0) - x;
}

template <uint64_t P, typename Word>
inline constexpr ModInt<P, Word> operator*(
    UInt lhs, const ModInt<P, Word>& rhs) noexcept {
  return ModInt<P, Word>(
      static_cast<typename ModInt<P, Word>::raw_value_type>(lhs) * rhs.value());
}

#endif  // PRIMITIVE_MODINT_HPP