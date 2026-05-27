#ifndef TRLWE_HPP
#define TRLWE_HPP

#include <concepts>

#include "algebra/poly.hpp"
#include "primitive/concept/castable.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

template <typename T = Torus>
  requires std::same_as<T, Torus> || std::same_as<T, ModTorus>
class TRLWE {
 public:
  TRLWE(uint32_t N) : a_(N), b_(N) {}
  // NOLINT(bugprone-easily-swappable-parameters)
  TRLWE(const Poly<T>& a, const Poly<T>& b) : a_(a), b_(b) {}

  template <typename F>
    requires requires(F& f) {
      { std::invoke(f) } -> castable<T>;
    }
  TRLWE(uint32_t N, F&& f) : a_(N, std::forward<F>(f)), b_(N) {}

  Poly<T>& a() { return a_; }
  const Poly<T>& a() const { return a_; }
  Poly<T>& b() { return b_; }
  const Poly<T>& b() const { return b_; }

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
  Poly<T> a_;
  Poly<T> b_;
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

template <typename T>
  requires std::same_as<T, Torus> || std::same_as<T, ModTorus>
inline TRLWE<T> operator+(TRLWE<T> lhs, const TRLWE<T>& rhs) {
  lhs += rhs;
  return lhs;
}

template <typename T>
  requires std::same_as<T, Torus> || std::same_as<T, ModTorus>
inline TRLWE<T> operator-(TRLWE<T> lhs, const TRLWE<T>& rhs) {
  lhs -= rhs;
  return lhs;
}

#endif