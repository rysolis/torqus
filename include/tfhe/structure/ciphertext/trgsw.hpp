// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TRGSW_HPP
#define TFHE_TRGSW_HPP

#include <iostream>

#include "primitive/concept/primitive.hpp"
#include "primitive/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <torus_type Torus, uint32_t N, uint32_t l>
class TRGSW {
 public:
  TRGSW() : error_bound_(0.0) {}

  TRLWE<Torus, N>& operator[](size_t idx) noexcept { return trgsw_[idx]; }
  const TRLWE<Torus, N>& operator[](size_t idx) const noexcept {
    return trgsw_[idx];
  }

  double error_bound() const noexcept { return error_bound_; }
  void update_bound(double v) noexcept { error_bound_ = v; }

  uint32_t level() const noexcept { return 2 * l; }

  friend std::ostream& operator<<(std::ostream& os, const TRGSW& trgsw) {
    os << "TRGSW\n";
    for (size_t i = 0; i < trgsw.level(); ++i) {
      os << trgsw.trgsw_[i] << "\n";
    }
    os << "\n";
    return os;
  }

 private:
  Vector<TRLWE<Torus, N>, 2 * l> trgsw_;
  double error_bound_;
};

template <typename To, typename From, uint32_t N, uint32_t l>
  requires explicitly_convertible_to<To, From>
inline TRGSW<To, N, l> convert_to(const TRGSW<From, N, l>& src) {
  TRGSW<To, N, l> dst(static_cast<uint32_t>(src[0].a().size()),
                      src.level() >> 1);
  for (size_t i = 0; i < src.level(); ++i) {
    dst[i] = convert_to<To>(src[i]);
  }
  return dst;
}

#endif  // TFHE_TRGSW_HPP