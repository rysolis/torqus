#ifndef CASTABLE_HPP
#define CASTABLE_HPP

#include <concepts>
#include <utility>

template <typename To, typename From>
concept castable = requires(From f) { static_cast<To>(f); };

template <typename To, typename From>
concept same_representation =
    std::same_as<typename std::decay_t<To>::raw_value_type,
                 typename std::decay_t<From>::raw_value_type>;

#endif