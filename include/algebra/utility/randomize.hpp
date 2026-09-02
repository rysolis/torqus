// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_UTILITY_RANDOMIZE_HPP
#define ALGEBRA_UTILITY_RANDOMIZE_HPP

#include <concepts>
#include <cstdint>
#include <random>

#include "primitive/concept/torus.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

template <typename T>
struct default_distribution;

template <uint32_t QBit>
struct default_distribution<ModTorus<QBit>> {
  using type =
      std::uniform_int_distribution<typename ModTorus<QBit>::raw_value_type>;
};

template <uint64_t P>
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

// Samples a single Gaussian(0, alpha^2) error and rounds it onto the torus.
// Goes through dbl::Torus (canonical representative in [0, 1), already
// reduced mod 1 by its constructor) so it works uniformly for any
// torus_concept Torus -- ModTorus<QBit> included, via its existing
// dbl::Torus -> ModTorus<QBit> conversion -- without a separate rounding
// path per representation.
template <torus_concept Torus, typename Engine>
inline Torus gaussian_noise(Engine& eng, double alpha) {
  std::normal_distribution<double> dist(0.0, alpha);
  return static_cast<Torus>(dbl::Torus(dist(eng)));
}

#endif  // ALGEBRA_UTILITY_RANDOMIZE_HPP