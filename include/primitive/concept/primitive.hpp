// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_CONCEPT_HPP
#define PRIMITIVE_CONCEPT_HPP

#include <concepts>
#include <type_traits>
#include <utility>

// Primitive types exposed by the library.
// Internal helper types (e.g. detail::Torus) are intentionally excluded.
template <typename T>
concept primitive =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T> &&
    std::has_unique_object_representations_v<T>;

#endif  // PRIMITIVE_CONCEPT_HPP