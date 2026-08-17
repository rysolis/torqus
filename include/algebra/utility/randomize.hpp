// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_UTILITY_RANDOMIZE_HPP
#define ALGEBRA_UTILITY_RANDOMIZE_HPP

#include <concepts>
#include <cstdint>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

template <typename T>
struct default_distribution;

template <uint32_t QBit>
struct default_distribution<ModTorus<QBit>> {
  using type =
      std::uniform_int_distribution<typename ModTorus<QBit>::raw_value_type>;
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
inline void randomize(Container& container, Engine& eng) {
  using value_type = typename Container::value_type;
  for (size_t i = 0; i < container.size(); ++i) {
    container[i] = random_value<value_type>(eng);
  }
}

template <typename Container, typename Engine>
inline Container randomize(Engine& eng) {
  Container container;
  randomize(container, eng);
  return container;
}

#endif  // ALGEBRA_UTILITY_RANDOMIZE_HPP