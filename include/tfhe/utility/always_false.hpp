#ifndef TFHE_UTILITY_ALWAYS_FALSE_HPP
#define TFHE_UTILITY_ALWAYS_FALSE_HPP

template <typename...>
inline constexpr bool always_false_v = false;

#endif