// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_DETAIL_SIMD_OPS_HPP
#define ALGEBRA_DETAIL_SIMD_OPS_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>

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
// simd_lane::WideLane, and only when TORQUS_SIMD_HAS_HARDWARE_LANE says
// WideLane is a real hardware Lane (never ScalarLane). This file doesn't
// know or care which ISA that is -- adding AVX2/AVX512 support is a
// simd_lane.hpp-only change. Whatever a Lane pass doesn't cover (every
// element when no hardware Lane is compiled in, or the last
// `< Lane::width` elements when one is) is finished by calling
// ModInt<P>::operator+=/-= directly, so the reduction formula itself has
// exactly one implementation, not a second copy re-derived through Lane
// primitives.
template <uint64_t P, typename Word>
struct simd_ops<ModInt<P, Word>> {
  using raw_value_type = Word;

  // The hardware Lane strategies (simd_lane.hpp) only operate on uint32_t
  // elements today, so a 64-bit Word (see primitive/word.hpp's
  // TORQUS_TORUS_BITS=64 configuration) always takes the scalar tail loop
  // below -- no vectorized fast path yet for that width. `is_same_v` (not
  // sizeof) is checked directly so this is decided purely at compile time,
  // letting an `if constexpr(false)` branch below skip instantiating the
  // hardware Lane calls (which assume uint32_t*) entirely rather than
  // trying to compile them against a uint64_t buffer.
  static constexpr bool has_hardware_lane =
      TORQUS_SIMD_HAS_HARDWARE_LANE && std::is_same_v<raw_value_type, uint32_t>;

  template <typename Lane>
  static void add_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    if constexpr (P == 0) {
      for (; i + Lane::width <= n; i += Lane::width) {
        Lane::store(dst + i,
                    Lane::add(Lane::load(dst + i), Lane::load(src + i)));
      }
    } else {
      typename Lane::vec modv = Lane::broadcast(static_cast<uint32_t>(P));
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
      typename Lane::vec modv = Lane::broadcast(static_cast<uint32_t>(P));
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
    if constexpr (has_hardware_lane) {
      add_lanes<simd_lane::WideLane>(dst, src, i, n);
    }
    for (; i < n; ++i) {
      ModInt<P, Word> lhs = static_cast<ModInt<P, Word>>(dst[i]);
      lhs += static_cast<ModInt<P, Word>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }

  static void sub_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
    if constexpr (has_hardware_lane) {
      sub_lanes<simd_lane::WideLane>(dst, src, i, n);
    }
    for (; i < n; ++i) {
      ModInt<P, Word> lhs = static_cast<ModInt<P, Word>>(dst[i]);
      lhs -= static_cast<ModInt<P, Word>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }
};

// ModTorus<QBit>::operator+=/-= is add/sub followed by masking off the
// high bits (include/primitive/torus.hpp:160-171). Same split as
// ModInt<P> above: add_lanes/sub_lanes are the vectorized half (WideLane
// only, gated on TORQUS_SIMD_HAS_HARDWARE_LANE and Word being uint32_t --
// see ModInt's has_hardware_lane above for why), and whatever's left is
// finished by calling ModTorus<QBit>::operator+=/-= directly.
template <uint32_t QBit, typename Word>
struct simd_ops<ModTorus<QBit, Word>> {
  using raw_value_type = Word;

  static constexpr bool has_hardware_lane =
      TORQUS_SIMD_HAS_HARDWARE_LANE && std::is_same_v<raw_value_type, uint32_t>;

  template <typename Lane>
  static void add_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    typename Lane::vec maskv = Lane::broadcast(ModTorus<QBit, Word>::mask());
    for (; i + Lane::width <= n; i += Lane::width) {
      auto sum = Lane::add(Lane::load(dst + i), Lane::load(src + i));
      Lane::store(dst + i, Lane::bit_and(sum, maskv));
    }
  }

  template <typename Lane>
  static void sub_lanes(raw_value_type* dst, const raw_value_type* src,
                        std::size_t& i, std::size_t n) {
    typename Lane::vec maskv = Lane::broadcast(ModTorus<QBit, Word>::mask());
    for (; i + Lane::width <= n; i += Lane::width) {
      auto diff = Lane::sub(Lane::load(dst + i), Lane::load(src + i));
      Lane::store(dst + i, Lane::bit_and(diff, maskv));
    }
  }

  static void add_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
    if constexpr (has_hardware_lane) {
      add_lanes<simd_lane::WideLane>(dst, src, i, n);
    }
    for (; i < n; ++i) {
      ModTorus<QBit, Word> lhs = static_cast<ModTorus<QBit, Word>>(dst[i]);
      lhs += static_cast<ModTorus<QBit, Word>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }

  static void sub_assign(raw_value_type* dst, const raw_value_type* src,
                         std::size_t n) {
    std::size_t i = 0;
    if constexpr (has_hardware_lane) {
      sub_lanes<simd_lane::WideLane>(dst, src, i, n);
    }
    for (; i < n; ++i) {
      ModTorus<QBit, Word> lhs = static_cast<ModTorus<QBit, Word>>(dst[i]);
      lhs -= static_cast<ModTorus<QBit, Word>>(src[i]);
      dst[i] = static_cast<raw_value_type>(lhs);
    }
  }
};

#endif  // ALGEBRA_DETAIL_SIMD_OPS_HPP
