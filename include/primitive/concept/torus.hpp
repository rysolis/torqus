// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_TORUS_CONCEPT_HPP
#define PRIMITIVE_TORUS_CONCEPT_HPP

#include <concepts>

template <typename Torus>
class TorusBase;

template <typename Torus>
concept torus_type = std::derived_from<std::remove_cvref_t<Torus>,
                                       TorusBase<std::remove_cvref_t<Torus>>>;

#endif  // PRIMITIVE_TORUS_CONCEPT_HPP
