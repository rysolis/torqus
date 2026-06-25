#ifndef TRLWE_HPP
#define TRLWE_HPP

#include <concepts>

#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"
#include "primitive/concept/torus.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

#include "algebra/poly.hpp"

template <torus_type Torus, uint32_t N>
class TRLWE {
 public:
  TRLWE() = default;
  // NOLINT(bugprone-easily-swappable-parameters)
  TRLWE(const Poly<Torus, N>& a, const Poly<Torus, N>& b)
      : a_(a), b_(b), error_bound_(0.0001) {}

  template <typename F>
    requires requires(F& f) {
      { std::invoke(f) } -> explicitly_convertible_to<Torus>;
    }
  TRLWE(F&& f) : a_(std::forward<F>(f)) {}

  Poly<Torus, N>& a() { return a_; }
  const Poly<Torus, N>& a() const { return a_; }
  Poly<Torus, N>& b() { return b_; }
  const Poly<Torus, N>& b() const { return b_; }

  TRLWE& operator+=(const TRLWE& other) {
    a_ += other.a_;
    b_ += other.b_;
    error_bound_ += other.error_bound();
    return *this;
  }

  // |e1 - e2| <= |e1| + |e2|
  TRLWE& operator-=(const TRLWE& other) {
    a_ -= other.a_;
    b_ -= other.b_;
    error_bound_ += other.error_bound();
    return *this;
  }

  double error_bound() const noexcept { return error_bound_; }
  void update_bound(double v) noexcept { error_bound_ = v; }

  friend std::ostream& operator<<(std::ostream& os, const TRLWE& trlwe) {
    os << "TRLWE(a: " << trlwe.a_ << ", b: " << trlwe.b_ << ")";
    return os;
  }

 private:
  Poly<Torus, N> a_;
  Poly<Torus, N> b_;
  double error_bound_ = 0.0001;
};

template <typename To, typename From, uint32_t N>
  requires explicitly_convertible_to<To, From>
inline TRLWE<To, N> convert_to(TRLWE<From, N>&& src) {
  return TRLWE<To, N>(convert_to<To, N>(std::move(src.a())),
                      convert_to<To, N>(std::move(src.b())));
}

template <typename To, typename From, uint32_t N>
  requires explicitly_convertible_to<To, From>
inline TRLWE<To, N> convert_to(const TRLWE<From, N>& src) {
  return TRLWE<To, N>(convert_to<To, N>(src.a()), convert_to<To, N>(src.b()));
}

template <torus_type Torus, uint32_t N>
inline TRLWE<Torus, N> operator+(TRLWE<Torus, N> lhs,
                                 const TRLWE<Torus, N>& rhs) {
  lhs += rhs;
  return lhs;
}

template <torus_type Torus, uint32_t N>
inline TRLWE<Torus, N> operator-(TRLWE<Torus, N> lhs,
                                 const TRLWE<Torus, N>& rhs) {
  lhs -= rhs;
  return lhs;
}

#endif