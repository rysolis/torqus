#ifndef MODINT_HPP
#define MODINT_HPP

#include <cstdint>
#include <iostream>

template <uint32_t P>
class ModInt {
 public:
  static constexpr uint32_t MOD = P;
  using raw_value_type = uint32_t;

  constexpr ModInt() noexcept = default;

  template <std::integral Raw>
    requires std::convertible_to<Raw, raw_value_type>
  constexpr explicit ModInt(Raw v = 0) noexcept
      : v_(static_cast<raw_value_type>(v) % MOD) {}

  constexpr explicit operator raw_value_type() const noexcept { return v_; }

  constexpr ModInt& operator+=(const ModInt& rhs) noexcept {
    v_ += rhs.v_;
    ModInt::raw_value_type mask = -static_cast<uint32_t>(v_ >= P);
    v_ -= (MOD & mask);
    return *this;
  }
  // when a < b,
  // compute ((a - b) + 2^{32}) + P mod 2^{32} = (a - b) + P
  constexpr ModInt& operator-=(const ModInt& rhs) noexcept {
    ModInt::raw_value_type mask = -static_cast<uint32_t>(v_ < rhs.v_);
    v_ -= rhs.v_;        // when underflow occurred, compute (a - b) + 2^{32}
    v_ += (MOD & mask);  // when overflow accurred, implicity apply - 2^{32}
    return *this;
  }

  constexpr bool operator==(const ModInt& other) const noexcept {
    return v_ == other.v_;
  }

  friend std::ostream& operator<<(std::ostream& os, const ModInt& m) {
    return os << m.v_;
  }

 private:
  raw_value_type v_;
};

template <uint32_t P>
inline constexpr ModInt<P> operator+(ModInt<P> lhs, const ModInt<P>& rhs) {
  return lhs += rhs;
}

template <uint32_t P>
inline constexpr ModInt<P> operator-(ModInt<P> lhs, const ModInt<P>& rhs) {
  return lhs -= rhs;
}

#endif