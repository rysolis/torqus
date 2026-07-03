// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_CONVERTIBLE_CONCEPT_HPP
#define PRIMITIVE_CONVERTIBLE_CONCEPT_HPP

#include <concepts>
#include <utility>

template <typename From, typename To>
concept lvalue_explicitly_convertible_to =
    requires(From& x) { static_cast<To>(x); };

template <typename From, typename To>
concept rvalue_explicitly_convertible_to =
    requires(From&& x) { static_cast<To>(std::move(x)); };

template <typename From, typename To>
concept explicitly_convertible_to =
    lvalue_explicitly_convertible_to<From, To> ||
    rvalue_explicitly_convertible_to<From, To>;

#endif  // PRIMITIVE_CONVERTIBLE_CONCEPT_HPP