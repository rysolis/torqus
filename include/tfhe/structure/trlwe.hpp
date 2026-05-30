#ifndef TRLWE_HPP
#define TRLWE_HPP

#include <concepts>

#include "algebra/poly.hpp"
#include "primitive/concept/castable.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

template <TorusType Torus>
class TRLWE {
 public:
  TRLWE(uint32_t N) : a_(N), b_(N) {}
  // NOLINT(bugprone-easily-swappable-parameters)
  TRLWE(const Poly<Torus>& a, const Poly<Torus>& b) : a_(a), b_(b) {}

  template <typename F>
    requires requires(F& f) {
      { std::invoke(f) } -> castable<Torus>;
    }
  TRLWE(uint32_t N, F&& f) : a_(N, std::forward<F>(f)), b_(N) {}

  Poly<Torus>& a() { return a_; }
  const Poly<Torus>& a() const { return a_; }
  Poly<Torus>& b() { return b_; }
  const Poly<Torus>& b() const { return b_; }

  TRLWE& operator+=(const TRLWE& other) {
    a_ += other.a_;
    b_ += other.b_;
    return *this;
  }

  TRLWE& operator-=(const TRLWE& other) {
    a_ -= other.a_;
    b_ -= other.b_;
    return *this;
  }

  friend std::ostream& operator<<(std::ostream& os, const TRLWE& trlwe) {
    os << "TRLWE(a: " << trlwe.a_ << ", b: " << trlwe.b_ << ")";
    return os;
  }

 private:
  Poly<Torus> a_;
  Poly<Torus> b_;
};

template <typename To, typename From>
  requires castable<To, From>
inline TRLWE<To> convert_to(TRLWE<From>&& src) {
  return TRLWE<To>(convert_to<To>(std::move(src.a())),
                   convert_to<To>(std::move(src.b())));
}

template <typename To, typename From>
  requires castable<To, From>
inline TRLWE<To> convert_to(const TRLWE<From>& src) {
  return TRLWE<To>(convert_to<To>(src.a()), convert_to<To>(src.b()));
}

template <TorusType Torus>
// requires std::same_as<T, Torus> || std::same_as<T, ModTorus>
inline TRLWE<Torus> operator+(TRLWE<Torus> lhs, const TRLWE<Torus>& rhs) {
  lhs += rhs;
  return lhs;
}

template <TorusType Torus>
// requires std::same_as<T, Torus> || std::same_as<T, ModTorus>
inline TRLWE<Torus> operator-(TRLWE<Torus> lhs, const TRLWE<Torus>& rhs) {
  lhs -= rhs;
  return lhs;
}

#endif