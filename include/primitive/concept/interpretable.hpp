// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_INTERPRETABLE_CONCEPT_HPP
#define PRIMITIVE_INTERPRETABLE_CONCEPT_HPP

#include <concepts>

template <typename From, typename To>
concept interpretable_to_concept =
    std::same_as<typename std::decay_t<To>::raw_value_type,
                 typename std::decay_t<From>::raw_value_type>;

#endif  // PRIMITIVE_INTERPRETABLE_CONCEPT_HPP