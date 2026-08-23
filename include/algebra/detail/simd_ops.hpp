#ifndef ALGEBRA_DETAIL_SIMD_OPS_HPP
#define ALGEBRA_DETAIL_SIMD_OPS_HPP

#include <cstddef>
#include <cstdint>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

#include "algebra/detail/simd_lane.hpp"
#include "algebra/detail/storage_traits.hpp"

// simd_ops<T> provides bulk (whole-buffer) add/sub for the contiguous
// raw_value_type storage backing Poly<T,Size>/Vector<T,Size>. The primary
// template is a scalar fallback that reproduces T's own operator+=/-=
// element-by-element, used for any T without a specialization below (e.g.
// UInt).
template <typename T>
struct simd_ops {
  using raw_value_type = typename storage_traits<T>::raw_value_type;

  static void add_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      T lhs = static_cast<T>(dst[i]);
      lhs += static_cast<T>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }

  static void sub_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      T lhs = static_cast<T>(dst[i]);
      lhs -= static_cast<T>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }
};

// ModInt<P>::operator+=/-= is a branchless conditional add/subtract of
// `mod` (include/primitive/modint.hpp:38-62). add_lanes/sub_lanes below
// are the *vectorized* half of that, generic over a Lane strategy
// (algebra/detail/simd_lane.hpp) -- only ever instantiated with
// simd_lane::WideLane, and only when PPV_SIMD_HAS_HARDWARE_LANE says
// WideLane is a real hardware Lane (never ScalarLane). This file doesn't
// know or care which ISA that is -- adding AVX2/AVX512 support is a
// simd_lane.hpp-only change. Whatever a Lane pass doesn't cover (every
// element when no hardware Lane is compiled in, or the last
// `< Lane::width` elements when one is) is finished by calling
// ModInt<P>::operator+=/-= directly, so the reduction formula itself has
// exactly one implementation, not a second copy re-derived through Lane
// primitives.
template <uint32_t P>
struct simd_ops<ModInt<P>> {
  using raw_value_type = uint32_t;

  template <typename Lane>
  static void add_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    if constexpr (P == 0) {
      for (; i + Lane::width <= n; i += Lane::width) {
        Lane::store(dst + i,
                    Lane::add(Lane::load(dst + i), Lane::load(src + i)));
      }
    } else {
      typename Lane::vec modv = Lane::broadcast(P);
      for (; i + Lane::width <= n; i += Lane::width) {
        auto sum = Lane::add(Lane::load(dst + i), Lane::load(src + i));
        auto reduce = Lane::bit_and(modv, Lane::ge(sum, modv));
        Lane::store(dst + i, Lane::sub(sum, reduce));
      }
    }
  }

  template <typename Lane>
  static void sub_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    if constexpr (P == 0) {
      for (; i + Lane::width <= n; i += Lane::width) {
        Lane::store(dst + i,
                    Lane::sub(Lane::load(dst + i), Lane::load(src + i)));
      }
    } else {
      typename Lane::vec modv = Lane::broadcast(P);
      for (; i + Lane::width <= n; i += Lane::width) {
        auto a = Lane::load(dst + i);
        auto b = Lane::load(src + i);
        auto reduce = Lane::bit_and(modv, Lane::lt(a, b));
        Lane::store(dst + i, Lane::add(Lane::sub(a, b), reduce));
      }
    }
  }

  static void add_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
#if PPV_SIMD_HAS_HARDWARE_LANE
    add_lanes<simd_lane::WideLane>(dst, src, i, n);
#endif
    for (; i < n; ++i) {
      ModInt<P> lhs = static_cast<ModInt<P>>(dst[i]);
      lhs += static_cast<ModInt<P>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }

  static void sub_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
#if PPV_SIMD_HAS_HARDWARE_LANE
    sub_lanes<simd_lane::WideLane>(dst, src, i, n);
#endif
    for (; i < n; ++i) {
      ModInt<P> lhs = static_cast<ModInt<P>>(dst[i]);
      lhs -= static_cast<ModInt<P>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }
};

// ModTorus<QBit>::operator+=/-= is add/sub followed by masking off the
// high bits (include/primitive/torus.hpp:160-171). Same split as
// ModInt<P> above: add_lanes/sub_lanes are the vectorized half (WideLane
// only, gated on PPV_SIMD_HAS_HARDWARE_LANE), and whatever's left is
// finished by calling ModTorus<QBit>::operator+=/-= directly.
template <uint32_t QBit>
struct simd_ops<ModTorus<QBit>> {
  using raw_value_type = uint32_t;

  template <typename Lane>
  static void add_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    typename Lane::vec maskv = Lane::broadcast(ModTorus<QBit>::mask());
    for (; i + Lane::width <= n; i += Lane::width) {
      auto sum = Lane::add(Lane::load(dst + i), Lane::load(src + i));
      Lane::store(dst + i, Lane::bit_and(sum, maskv));
    }
  }

  template <typename Lane>
  static void sub_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    typename Lane::vec maskv = Lane::broadcast(ModTorus<QBit>::mask());
    for (; i + Lane::width <= n; i += Lane::width) {
      auto diff = Lane::sub(Lane::load(dst + i), Lane::load(src + i));
      Lane::store(dst + i, Lane::bit_and(diff, maskv));
    }
  }

  static void add_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
#if PPV_SIMD_HAS_HARDWARE_LANE
    add_lanes<simd_lane::WideLane>(dst, src, i, n);
#endif
    for (; i < n; ++i) {
      ModTorus<QBit> lhs = static_cast<ModTorus<QBit>>(dst[i]);
      lhs += static_cast<ModTorus<QBit>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }

  static void sub_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
#if PPV_SIMD_HAS_HARDWARE_LANE
    sub_lanes<simd_lane::WideLane>(dst, src, i, n);
#endif
    for (; i < n; ++i) {
      ModTorus<QBit> lhs = static_cast<ModTorus<QBit>>(dst[i]);
      lhs -= static_cast<ModTorus<QBit>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }
};

#endif  // ALGEBRA_DETAIL_SIMD_OPS_HPP
