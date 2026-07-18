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
  { T::B } -> std::convertible_to<uint32_t>;
  { T::l } -> std::convertible_to<uint32_t>;
};

template <typename T>
concept kst_concept = requires {
  { T::K } -> std::convertible_to<uint32_t>;
  { T::t } -> std::convertible_to<uint32_t>;
};

#endif  // TFHE_CONCEPT_HPP