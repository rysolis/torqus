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
  constexpr explicit ModInt(Raw v = 0) noexcept {
    if constexpr (MOD == 0) {
      v_ = static_cast<raw_value_type>(v);
    } else {
      v_ = static_cast<raw_value_type>(v) % MOD;
    }
  }

  constexpr explicit operator raw_value_type() const noexcept { return v_; }
  constexpr raw_value_type value() const noexcept { return v_; }

  constexpr ModInt& operator+=(const ModInt& rhs) noexcept {
    if constexpr (MOD == 0) {
      v_ += rhs.v_;
    } else {
      v_ += rhs.v_;
      raw_value_type mask = -static_cast<raw_value_type>(v_ >= MOD);
      v_ -= (MOD & mask);
    }
    return *this;
  }
  // when a < b,
  // compute ((a - b) + 2^{32}) + P mod 2^{32} = (a - b) + P
  constexpr ModInt& operator-=(const ModInt& rhs) noexcept {
    if constexpr (MOD == 0) {
      v_ -= rhs.v_;
    } else {
      raw_value_type mask = -static_cast<raw_value_type>(v_ < rhs.v_);
      v_ -= rhs.v_;
      v_ += (MOD & mask);
    }
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
constexpr ModInt<P> operator-(const ModInt<P>& x) noexcept {
  return ModInt<P>(0) - x;
}

#endif