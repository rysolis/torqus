// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_MODSWITCH_HPP
#define TFHE_MODSWITCH_HPP

#include <cstdint>
#include <limits>

#include "primitive/modint.hpp"

namespace {
template <uint32_t N>
concept power_of_two_concept = (N & (N - 1)) == 0;
}

template <uint32_t M, uint32_t N>
  requires power_of_two_concept<N>
constexpr ModInt<M> mod_switch(const ModInt<N>& t) {
  constexpr uint32_t bits = std::bit_width(N - 1);
  constexpr uint32_t half =
      N == 0 ? uint32_t{1} << (std::numeric_limits<uint32_t>::digits - 1)
             : N >> 1;
  return ModInt<M>(((uint64_t(t.value()) * M) + half) >> bits);
}

#endif  // TFHE_MODSWITCH_HPP