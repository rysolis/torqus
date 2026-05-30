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

template <uint32_t QBit>
class ModTorus;

namespace detail {

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

  static constexpr raw_value_type raw_min() { return 0.0; }
  static constexpr raw_value_type raw_max() { return 1.0; }

  template <uint32_t QBit>
  constexpr explicit operator ModTorus<QBit>() const noexcept;

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

}  // namespace detail

/**
 * \brief Represents the integer torus $\hat{\mathbb{T}}$.
 *
 * The following correspondence holds:
 *
 * $\hat{\mathbb{T}} \ni c \sim q \cdot t \in (1/q)\mathbb{R}/\mathbb{Z}$.
 */
template <uint32_t QBit>
class ModTorus : public TorusBase<ModTorus<QBit>> {
 public:
  using raw_value_type = uint32_t;
  constexpr static uint32_t qbit = QBit;
  constexpr static uint32_t q = 1 << QBit;

  constexpr explicit ModTorus(raw_value_type m = 0) noexcept : m_(m % q) {}

  static constexpr raw_value_type raw_min() { return 0; }
  static constexpr raw_value_type raw_max() { return q - 1; }

  constexpr explicit operator detail::Torus() const noexcept;

  constexpr explicit operator raw_value_type() const noexcept { return m_; }
  constexpr raw_value_type value() const noexcept { return m_; }

  bool operator==(const ModTorus<QBit>& other) const noexcept {
    return this->self().value() == other.value();
  }

  constexpr ModTorus<QBit>& operator+=(const ModTorus<QBit>& other) noexcept {
    m_ += other.m_;
    m_ %= q;
    return *this;
  }

  constexpr ModTorus<QBit>& operator-=(const ModTorus<QBit>& other) noexcept {
    m_ -= other.m_;  // Since Q is a power of two, using an unsigned
                     // integer for m_ is safe.
    m_ %= q;
    return *this;
  }

 private:
  raw_value_type m_;
};

template <uint32_t QBit>
inline constexpr detail::Torus::operator ModTorus<QBit>() const noexcept {
  return ModTorus<QBit>(static_cast<ModTorus<QBit>::raw_value_type>(
      ModTorus<QBit>::q * this->value()));
}

template <uint32_t QBit>
inline constexpr ModTorus<QBit>::operator detail::Torus() const noexcept {
  return detail::Torus(
      static_cast<detail::Torus::raw_value_type>(this->value()) /
      ModTorus<QBit>::q);
}

inline constexpr detail::Torus operator+(detail::Torus lhs,
                                         const detail::Torus& rhs) noexcept {
  return lhs += rhs;
}

inline constexpr detail::Torus operator-(detail::Torus lhs,
                                         const detail::Torus& rhs) noexcept {
  return lhs -= rhs;
}

inline constexpr detail::Torus operator*(UInt lhs,
                                         const detail::Torus& rhs) noexcept {
  return detail::Torus(static_cast<UInt::raw_value_type>(lhs) * rhs.value());
}

template <uint32_t QBit>
inline constexpr ModTorus<QBit> operator*(UInt lhs,
                                          const ModTorus<QBit>& rhs) noexcept {
  return ModTorus<QBit>(static_cast<ModTorus<QBit>::raw_value_type>(lhs) *
                        rhs.value());
}

template <uint32_t QBit>
inline constexpr ModTorus<QBit> operator+(ModTorus<QBit> lhs,
                                          const ModTorus<QBit>& rhs) noexcept {
  return lhs += rhs;
}

template <uint32_t QBit>
inline constexpr ModTorus<QBit> operator-(ModTorus<QBit> lhs,
                                          const ModTorus<QBit>& rhs) noexcept {
  return lhs -= rhs;
}

inline constexpr double infinity_norm(const detail::Torus& torus) {
  double tmp = static_cast<double>(torus);
  return std::min(tmp, 1 - tmp);
}

template <uint32_t QBit>
inline constexpr double infinity_norm(const ModTorus<QBit>& mod_torus) {
  typename detail::Torus::raw_value_type tmp =
      detail::Torus::raw_value_type(static_cast<detail::Torus>(mod_torus));
  return std::min(tmp, 1 - tmp);
}

// Torus::operator== depends on infinity_norm, which is defined in
// utility.hpp, so we define it here to avoid circular dependency
inline bool detail::Torus::operator==(
    const detail::Torus& other) const noexcept {
  const double eps = 1e-9;
  double norm = infinity_norm((*this) - other);
  return norm < eps;
}

template <typename Torus>
concept TorusType = std::derived_from<std::remove_cvref_t<Torus>,
                                      TorusBase<std::remove_cvref_t<Torus>>>;

#endif