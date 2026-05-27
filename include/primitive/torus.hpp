#ifndef TORUS_HPP
#define TORUS_HPP

#include <cmath>
#include <concepts>
#include <cstdint>
#include <iostream>

#include "primitive/uint.hpp"

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
  static constexpr uint32_t BIT = 16;
  static constexpr uint32_t Q = 1 << BIT;

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

class ModTorus;

/**
 * \brief Represents the discrete torus $(1/q)\mathbb{R}/\mathbb{Z}$.
 *
 * The canonical representative is taken in the interval [0, 1).
 */
class Torus : public TorusBase<Torus> {
 public:
  using raw_value_type = double;

  constexpr explicit Torus(raw_value_type t = 0) noexcept
      : r_(t - std::floor(t)) {}

  constexpr explicit operator ModTorus() const noexcept;

  constexpr explicit operator raw_value_type() const noexcept { return r_; }
  constexpr raw_value_type value() const noexcept { return r_; }

  bool operator==(const Torus& other) const noexcept;

  constexpr Torus& operator+=(const Torus& other) noexcept {
    r_ += other.r_;
    r_ -= std::floor(r_);
    return *this;
  }

  constexpr Torus& operator-=(const Torus& other) noexcept {
    r_ -= other.r_;
    r_ -= std::floor(r_);
    return *this;
  }

 private:
  raw_value_type r_;
};

/**
 * \brief Represents the integer torus $\hat{\mathbb{T}}$.
 *
 * The following correspondence holds:
 *
 * $\hat{\mathbb{T}} \ni c \sim q \cdot t \in (1/q)\mathbb{R}/\mathbb{Z}$.
 */
class ModTorus : public TorusBase<ModTorus> {
 public:
  using raw_value_type = uint32_t;

  constexpr explicit ModTorus(raw_value_type m = 0) noexcept : m_(m % Q) {}

  constexpr explicit operator Torus() const noexcept;

  constexpr explicit operator raw_value_type() const noexcept { return m_; }
  constexpr raw_value_type value() const noexcept { return m_; }

  bool operator==(const ModTorus& other) const noexcept {
    return self().value() == other.value();
  }

  constexpr ModTorus& operator+=(const ModTorus& other) noexcept {
    m_ += other.m_;
    m_ %= Q;
    return *this;
  }

  constexpr ModTorus& operator-=(const ModTorus& other) noexcept {
    m_ -= other.m_;  // Since Q is a power of two, using an unsigned
                     // integer for m_ is safe.
    m_ %= Q;
    return *this;
  }

 private:
  raw_value_type m_;
};

inline constexpr Torus::operator ModTorus() const noexcept {
  return ModTorus(static_cast<ModTorus::raw_value_type>(Q * this->value()));
}

inline constexpr ModTorus::operator Torus() const noexcept {
  return Torus(static_cast<Torus::raw_value_type>(this->value()) / Q);
}

inline constexpr Torus operator+(Torus lhs, const Torus& rhs) noexcept {
  return lhs += rhs;
}

inline constexpr Torus operator-(Torus lhs, const Torus& rhs) noexcept {
  return lhs -= rhs;
}

inline constexpr Torus operator*(UInt lhs, const Torus& rhs) noexcept {
  return Torus(static_cast<UInt::raw_value_type>(lhs) * rhs.value());
}

inline constexpr ModTorus operator*(UInt lhs, const ModTorus& rhs) noexcept {
  return ModTorus(static_cast<ModTorus::raw_value_type>(lhs) * rhs.value());
}

inline constexpr ModTorus operator+(ModTorus lhs,
                                    const ModTorus& rhs) noexcept {
  return lhs += rhs;
}

inline constexpr ModTorus operator-(ModTorus lhs,
                                    const ModTorus& rhs) noexcept {
  return lhs -= rhs;
}

template <typename T>
  requires std::same_as<std::decay_t<T>, Torus>
inline constexpr double infinity_norm(const T& torus) {
  double tmp = static_cast<double>(torus);
  return std::min(tmp, 1 - tmp);
}

template <typename T>
  requires std::same_as<std::decay_t<T>, ModTorus>
inline constexpr double infinity_norm(const T& mod_torus) {
  Torus::raw_value_type tmp =
      static_cast<Torus::raw_value_type>(static_cast<Torus>(mod_torus));
  return std::min(tmp, 1 - tmp);
}

// Torus::operator== depends on infinity_norm, which is defined in utility.hpp,
// so we define it here to avoid circular dependency
inline bool Torus::operator==(const Torus& other) const noexcept {
  const double eps = 1e-9;
  double norm = infinity_norm((*this) - other);
  return norm < eps;
}

#endif