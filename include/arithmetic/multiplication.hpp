#ifndef MULTIPLICATION_HPP
#define MULTIPLICATION_HPP

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

template <typename T>
  requires std::derived_from<T, TorusBase<T>>
inline Poly<T> operator*(Poly<UInt> lhs, const Poly<T>& rhs) {
  assert(lhs.size() == rhs.size());
  const size_t N = lhs.size();
  Poly<T> res(N);

  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      size_t k = i + j;

      if (k < N) {
        res[k] += static_cast<UInt>(lhs[i]) * static_cast<T>(rhs[j]);
      } else {
        res[k - N] -= static_cast<UInt>(lhs[i]) * static_cast<T>(rhs[j]);
      }
    }
  }

  return res;
}

#endif