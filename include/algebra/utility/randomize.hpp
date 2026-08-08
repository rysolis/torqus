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

template <typename T, typename Engine>
inline T random_value(Engine& eng) {
  default_distribution_t<T> dist(T::raw_min(), T::raw_max());
  return static_cast<T>(dist(eng));
}

template <typename Container, typename Engine>
inline void randomize(Container& ctn, Engine& eng) {
  using value_type = typename Container::value_type;
  for (size_t i = 0; i < ctn.size(); ++i) {
    ctn[i] = random_value<value_type>(eng);
  }
}

template <typename Container, typename Engine>
inline Container randomize(Engine& eng) {
  Container ctn;
  randomize(ctn, eng);
  return ctn;
}

#endif  // UTILITY_RANDOMIZE_HPP