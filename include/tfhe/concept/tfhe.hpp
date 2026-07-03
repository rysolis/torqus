// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CONCEPT_HPP
#define TFHE_CONCEPT_HPP

#include <concepts>
#include <cstdint>

template <typename T>
concept tlwe_concept = requires {
  typename T::torus_type;
  { T::n } -> std::convertible_to<uint32_t>;
};

template <typename T>
concept trlwe_concept = requires {
  typename T::torus_type;
  { T::N } -> std::convertible_to<uint32_t>;
};

template <typename T>
concept decompose_concept = requires {
  { T::N } -> std::convertible_to<uint32_t>;
  { T::B } -> std::convertible_to<uint32_t>;
  { T::l } -> std::convertible_to<uint32_t>;
};

template <typename T>
concept trgsw_concept = trlwe_concept<T> && decompose_concept<T>;

#endif  // TFHE_CONCEPT_HPP