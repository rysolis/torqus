#ifndef params_HPP
#define params_HPP

#include <cstdint>

namespace decompose {

struct tag {};

template <typename Ctx>
struct params : tag {
  static constexpr uint32_t N = Ctx::N;
  static constexpr uint32_t B = Ctx::B;
  static constexpr uint32_t l = Ctx::l;
};

}  // namespace decompose

namespace trlwe {

struct tag {};

template <typename Ctx>
struct params : tag {
  static constexpr uint32_t N = Ctx::N;
};

}  // namespace trlwe

namespace trgsw {

struct tag {};

template <typename Ctx>
struct params : tag {
  static constexpr uint32_t N = Ctx::N;
  static constexpr uint32_t B = Ctx::B;
  static constexpr uint32_t l = Ctx::l;
};

}  // namespace trgsw

#endif