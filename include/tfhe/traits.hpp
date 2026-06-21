#ifndef TFHE_TRAITS_HPP
#define TFHE_TRAITS_HPP

#include <concepts>
#include <cstdint>

struct backend_tag {};
struct frontend_tag {};

template <typename T>
struct traits;

template <typename T>
  requires std::derived_from<T, backend_tag>
struct traits<T> {
  using Torus = typename T::core::Torus;
  static constexpr uint32_t N = T::core::N;
  static constexpr uint32_t B = T::gadget::B;
  static constexpr uint32_t l = T::gadget::l;
};

template <typename T>
  requires std::derived_from<T, frontend_tag>
struct traits<T> {
  using Torus = typename T::core::Torus;
  static constexpr uint32_t n = T::core::n;
};

#endif
