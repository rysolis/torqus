// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef UTILITY_RANDOMIZE_HPP
#define UTILITY_RANDOMIZE_HPP

#include <concepts>
#include <cstdint>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

template <typename T>
struct default_distribution;

template <uint32_t q>
struct default_distribution<ModTorus<q>> {
  using type =
      std::uniform_int_distribution<typename ModTorus<q>::raw_value_type>;
};

template <uint32_t P>
struct default_distribution<ModInt<P>> {
  using type =
      std::uniform_int_distribution<typename ModInt<P>::raw_value_type>;
};

template <typename T>
using default_distribution_t = typename default_distribution<T>::type;

template <typename T, uint32_t N>
class Poly;

template <typename T, uint32_t N, typename Engine,
          typename Dist = default_distribution_t<T>>
inline void randomize(Poly<T, N>& poly, Engine& eng) {
  Dist dist(T::raw_min(), T::raw_max());
  for (size_t i = 0; i < poly.size(); ++i) {
    poly[i] = static_cast<T>(dist(eng));
  }
}

template <typename T, uint32_t n>
class Vector;

template <typename T, uint32_t n, typename Engine,
          typename Dist = default_distribution_t<T>>
inline void randomize(Vector<T, n>& vec, Engine& eng) {
  Dist dist(T::raw_min(), T::raw_max());
  for (size_t i = 0; i < vec.size(); ++i) {
    vec[i] = static_cast<T>(dist(eng));
  }
}

#endif  // UTILITY_RANDOMIZE_HPP