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

template <typename Container, typename Engine>
inline void randomize(Container& ctn, Engine& eng) {
  using value_type = typename Container::value_type;
  default_distribution_t<value_type> dist(value_type::raw_min(),
                                          value_type::raw_max());
  for (size_t i = 0; i < ctn.size(); ++i) {
    ctn[i] = static_cast<value_type>(dist(eng));
  }
}

#endif  // UTILITY_RANDOMIZE_HPP