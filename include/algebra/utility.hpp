#include "algebra/poly.hpp"
#include "primitive/concept/convertible.hpp"
#include "primitive/concept/interpretable.hpp"
#include "primitive/concept/torus.hpp"

template <torus_type Torus, uint32_t N>
inline constexpr double infinity_norm(const Poly<Torus, N>& poly) {
  double max_norm = 0.0;
  for (size_t i = 0; i < poly.size(); ++i) {
    double abs_val = infinity_norm(static_cast<Torus>(poly[i]));
    if (abs_val > max_norm) {
      max_norm = abs_val;
    }
  }
  return max_norm;
}

template <typename To, typename From, uint32_t N>
  requires explicitly_convertible_to<To, From>
inline Poly<To, N> convert_to(const Poly<From, N>& src) {
  return Poly<To, N>([](const typename From::raw_value_type& v) {
    return static_cast<typename To::raw_value_type>(
        static_cast<To>(static_cast<From>(v)));
  });
}

template <typename To, typename From, uint32_t N>
  requires interpretable_to<To, From>
Poly<To, N> interpret_as(const Poly<From, N>& src) {
  return Poly<To, N>(src.begin(), src.end());
}

template <typename To, typename From, uint32_t N>
  requires interpretable_to<To, From>
Poly<To, N> interpret_as(Poly<From, N>&& src) {
  return Poly<To, N>(src.release());
}