// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TRGSW_HPP
#define TFHE_TRGSW_HPP

#include <iostream>

#include "primitive/concept/primitive.hpp"
#include "primitive/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

template <torus_type Torus, uint32_t N, uint32_t l>
class TRGSW {
 public:
  TRGSW() = default;
  TRLWE<Torus, N>& operator[](size_t idx) noexcept { return trgsw_[idx]; }
  const TRLWE<Torus, N>& operator[](size_t idx) const noexcept {
    return trgsw_[idx];
  }

  const void* identity() const noexcept { return trgsw_[0].identity(); }

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

namespace trgsw {

template <typename Rlwe, typename Dcp, torus_type Torus, typename Cryptor>
  requires trlwe_concept<Rlwe> && decompose_concept<Dcp>
TRGSW<Torus, Rlwe::N, Dcp::l> encrypt(Cryptor& cryptor,
                                      const Poly<UInt, Rlwe::N>& pt) {
  constexpr uint32_t N = Rlwe::N;
  constexpr uint32_t B = Dcp::B;
  constexpr uint32_t l = Dcp::l;

  TRGSW<Torus, N, l> ct;

  for (size_t i = 0; i < l; ++i) {
    ct[i] = cryptor.encrypt(ct[i].a());
    ct[l + i] = cryptor.encrypt(ct[l + i].a());

    detail::Torus v(static_cast<detail::Torus::raw_value_type>(
                        static_cast<UInt::raw_value_type>(pt[0])) /
                    (std::pow(B, i + 1)));
    Torus m = static_cast<Torus>(v);

    ct[i].a()[0] = static_cast<Torus>(ct[i].a()[0]) + m;
    ct[l + i].b()[0] = static_cast<Torus>(ct[l + i].b()[0]) + m;
  }
  return ct;
}

}  // namespace trgsw
#endif  // TFHE_TRGSW_HPP