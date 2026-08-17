// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_STRUCTURE_CIPHERTEXT_CONCEPTS_HPP
#define TFHE_STRUCTURE_CIPHERTEXT_CONCEPTS_HPP

#include <type_traits>

#include "primitive/concept/torus.hpp"

#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <typename T>
struct is_tlwe : std::false_type {};

template <torus_concept Torus, uint32_t n>
struct is_tlwe<TLWE<Torus, n>> : std::true_type {};

template <typename T>
concept tlwe_ciphertext_concept = is_tlwe<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_trlwe : std::false_type {};

template <torus_concept Torus, uint32_t N>
struct is_trlwe<TRLWE<Torus, N>> : std::true_type {};

template <typename T>
concept trlwe_ciphertext_concept = is_trlwe<std::remove_cvref_t<T>>::value;

template <typename T>
struct is_trgsw : std::false_type {};

template <torus_concept Torus, uint32_t N, uint32_t l>
struct is_trgsw<TRGSW<Torus, N, l>> : std::true_type {};

template <typename T>
concept trgsw_ciphertext_concept = is_trgsw<std::remove_cvref_t<T>>::value;

#endif  // TFHE_STRUCTURE_CIPHERTEXT_CONCEPTS_HPP
