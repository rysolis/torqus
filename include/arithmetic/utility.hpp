#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <concepts>

template <typename T>
class Poly;

template <typename P, typename Engine, typename Dist,
          typename T = typename std::decay_t<P>::value_type>
  requires std::same_as<std::decay_t<P>, Poly<T>>
inline constexpr void randomize(P&& poly, Engine& eng, Dist& dist) {
  for (size_t i = 0; i < poly.size(); ++i) {
    poly[i] = static_cast<T>(dist(eng));
  }
}

#endif