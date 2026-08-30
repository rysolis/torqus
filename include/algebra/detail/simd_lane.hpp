// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_DETAIL_SIMD_LANE_HPP
#define ALGEBRA_DETAIL_SIMD_LANE_HPP

#include <cstddef>
#include <cstdint>
#include <string>

// TORQUS_DISABLE_SIMD (set via the TORQUS_ENABLE_SIMD=OFF CMake option, see
// CMakeLists.txt) forces the scalar Lane path even when the compiler
// could do NEON -- an escape hatch/A-B switch, not something this header
// decides on its own.
#if defined(__ARM_NEON) && !defined(TORQUS_DISABLE_SIMD)
#define TORQUS_SIMD_NEON_ENABLED 1
#include <arm_neon.h>
#else
#define TORQUS_SIMD_NEON_ENABLED 0
#endif

namespace simd_lane {

// Strategy interface: a Lane processes `width` uint32_t elements as one
// unit, via the operations below. Real hardware backends implement this
// interface to vectorize the bulk of a buffer in simd_ops
// (algebra/detail/simd_ops.hpp) -- NeonLane today; AVX2/AVX512 Lanes are
// wanted here too, not implemented yet. Whatever a hardware Lane's pass
// doesn't cover (every element when none is compiled in, or the last
// `< width` elements when one is), simd_ops finishes by calling the
// element type's own operator+=/-= directly, not a Lane -- so the actual
// reduction formula (e.g. ModInt<P>'s conditional subtract) has exactly
// one implementation, not a second copy re-derived through Lane
// primitives.
//
// ScalarLane (width 1) is the reference implementation of the interface
// itself: the simplest possible one, useful as a template when writing a
// new hardware Lane. simd_ops never instantiates it for computation --
// WideLane below exists purely so active_lane_description() can report
// "Scalar" when no hardware Lane is compiled in.
struct ScalarLane {
  static constexpr std::size_t width = 1;
  using vec = uint32_t;

  static constexpr const char* name() noexcept { return "Scalar"; }

  static vec load(const uint32_t* p) noexcept { return *p; }
  static void store(uint32_t* p, vec v) noexcept { *p = v; }
  static vec add(vec a, vec b) noexcept { return a + b; }
  static vec sub(vec a, vec b) noexcept { return a - b; }
  static vec broadcast(uint32_t v) noexcept { return v; }
  // ge/lt return an all-ones/all-zeros mask, mirroring a NEON compare.
  static vec ge(vec a, vec b) noexcept {
    return a >= b ? ~uint32_t{0} : uint32_t{0};
  }
  static vec lt(vec a, vec b) noexcept {
    return a < b ? ~uint32_t{0} : uint32_t{0};
  }
  static vec bit_and(vec a, vec b) noexcept { return a & b; }
};

#if TORQUS_SIMD_NEON_ENABLED
struct NeonLane {
  static constexpr std::size_t width = 4;
  using vec = uint32x4_t;

  static constexpr const char* name() noexcept { return "NEON"; }

  static vec load(const uint32_t* p) noexcept { return vld1q_u32(p); }
  static void store(uint32_t* p, vec v) noexcept { vst1q_u32(p, v); }
  static vec add(vec a, vec b) noexcept { return vaddq_u32(a, b); }
  static vec sub(vec a, vec b) noexcept { return vsubq_u32(a, b); }
  static vec broadcast(uint32_t v) noexcept { return vdupq_n_u32(v); }
  static vec ge(vec a, vec b) noexcept { return vcgeq_u32(a, b); }
  static vec lt(vec a, vec b) noexcept { return vcltq_u32(a, b); }
  static vec bit_and(vec a, vec b) noexcept { return vandq_u32(a, b); }
};
#endif

// A new ISA backend (e.g. AVX2/AVX512) adds its own TORQUS_SIMD_xxx_ENABLED
// block above (a Lane struct guarded by its own `#if defined(__AVXxxx__)`,
// same shape as NeonLane) and joins the priority chain below. Nothing
// outside this file needs to change: simd_ops.hpp only ever asks for
// "the best available hardware Lane" via WideLane, gated by
// TORQUS_SIMD_HAS_HARDWARE_LANE -- it doesn't know NEON exists, and won't
// need to know AVX2/AVX512 exist either.
#if TORQUS_SIMD_NEON_ENABLED
using WideLane = NeonLane;
#define TORQUS_SIMD_HAS_HARDWARE_LANE 1
#else
using WideLane = ScalarLane;
#define TORQUS_SIMD_HAS_HARDWARE_LANE 0
#endif

// e.g. "NEON (width=4)" or "Scalar (width=1)" -- the same compile-time
// choice simd_ops<ModInt<P>>/<ModTorus<QBit>> dispatch on, exposed as text
// so callers (a startup log, etc.) can report it without reaching into
// the Lane types themselves.
inline const std::string& active_lane_description() {
  static const std::string description = [] {
    return std::string(WideLane::name()) +
           " (width=" + std::to_string(WideLane::width) + ")";
  }();
  return description;
}

}  // namespace simd_lane

#endif  // ALGEBRA_DETAIL_SIMD_LANE_HPP
