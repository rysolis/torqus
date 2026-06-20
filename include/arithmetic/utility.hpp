#ifndef ARITHMETIC_UTILITY_HPP
#define ARITHMETIC_UTILITY_HPP

#include <concepts>
#include <cstdint>

template <typename T, uint32_t N>
class Poly;

template <typename T, uint32_t N, typename Engine, typename Dist>
inline constexpr void randomize(Poly<T, N>& poly, Engine& eng, Dist& dist) {
  for (size_t i = 0; i < poly.size(); ++i) {
    poly[i] = static_cast<T>(dist(eng));
  }
}

template <typename T, uint32_t n>
class Vector;

template <typename T, uint32_t n, typename Engine, typename Dist>
inline constexpr void randomize(Vector<T, n>& vec, Engine& eng, Dist& dist) {
  for (size_t i = 0; i < vec.size(); ++i) {
    vec[i] = static_cast<T>(dist(eng));
  }
}

#endif