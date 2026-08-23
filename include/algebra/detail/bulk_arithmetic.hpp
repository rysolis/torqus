// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_DETAIL_BULK_ARITHMETIC_HPP
#define ALGEBRA_DETAIL_BULK_ARITHMETIC_HPP

#include <cstddef>

#include "algebra/detail/simd_ops.hpp"
#include "algebra/detail/storage_traits.hpp"

// Bulk (whole-buffer) add/sub for the contiguous raw storage backing
// Poly<T,Size>/Vector<T,Size>. This is the one place Poly/Vector reach
// for arithmetic -- it hides simd_ops<T> (the NEON/scalar dispatch,
// algebra/detail/simd_ops.hpp) behind two plain functions so callers
// never need to name simd_ops themselves. Container (algebra/container.hpp)
// is pure buffer management and doesn't include this file: it has no more
// reason to know an add/sub strategy exists than simd_ops has to know
// which ISA it's compiling for.
template <typename T>
void bulk_add_assign(typename storage_traits<T>::raw_value_type* dst,
                     const typename storage_traits<T>::raw_value_type* src,
                     std::size_t n) {
  simd_ops<T>::add_assign(dst, src, n);
}

template <typename T>
void bulk_sub_assign(typename storage_traits<T>::raw_value_type* dst,
                     const typename storage_traits<T>::raw_value_type* src,
                     std::size_t n) {
  simd_ops<T>::sub_assign(dst, src, n);
}

#endif  // ALGEBRA_DETAIL_BULK_ARITHMETIC_HPP
