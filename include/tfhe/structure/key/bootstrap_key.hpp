// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BOOTSTRAP_KEY_HPP
#define TFHE_BOOTSTRAP_KEY_HPP

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"

template <torus_type Torus, uint32_t N_, uint32_t l_, uint32_t n_>
class BootstrapKey {
 public:
  static constexpr uint32_t N = N_;
  static constexpr uint32_t l = l_;
  static constexpr uint32_t n = n_;

  BootstrapKey() = default;
  TRGSW<Torus, N, l>& operator[](size_t idx) noexcept { return bks_[idx]; }
  const TRGSW<Torus, N, l>& operator[](size_t idx) const noexcept {
    return bks_[idx];
  }

  const void* identity() const noexcept { return bks_[0].identity(); }

 private:
  Vector<TRGSW<Torus, N, l>, n> bks_;
};

namespace bootstrap_key {

template <typename Lwe, typename Rlwe, typename Dcp, typename Cryptor>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Dcp>
BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Dcp::l,
             Lwe::n> static generate(Cryptor& cryptor,
                                     const UInt::raw_value_type* s) {
  using rTorus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);
  static constexpr uint32_t l = Dcp::l;
  static constexpr uint32_t n = Lwe::n;

  BootstrapKey<rTorus, N, l, n> BK;
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < l; ++j) {
      BK[i][j] = cryptor.encrypt(Poly<rTorus, N>());
      BK[i][l + j] = cryptor.encrypt(Poly<rTorus, N>());

      rTorus v(static_cast<UInt::raw_value_type>(s[i]), 1u << (Bbit * (j + 1)));

      BK[i][j].a()[0] = static_cast<rTorus>(BK[i][j].a()[0]) + v;
      BK[i][l + j].b()[0] = static_cast<rTorus>(BK[i][l + j].b()[0]) + v;
    }
  }

  return BK;
}

}  // namespace bootstrap_key

#endif  // TFHE_BOOTSTRAP_KEY_HPP