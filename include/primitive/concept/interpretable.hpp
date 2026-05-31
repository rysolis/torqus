#ifndef PRIMITIVE_CONCEPT_INTERPRETABLE_HPP
#define PRIMITIVE_CONCEPT_INTERPRETABLE_HPP

#include <concepts>

template <typename To, typename From>
concept interpretable_to =
    std::same_as<typename std::decay_t<To>::raw_value_type,
                 typename std::decay_t<From>::raw_value_type>;
#endif