// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BOOTSTRAP_KEY_HPP
#define TFHE_BOOTSTRAP_KEY_HPP

#include "algebra/vector.hpp"

#include "tfhe/structure/ciphertext/trgsw.hpp"

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

 private:
  Vector<TRGSW<Torus, N, l>, n> bks_;
};

namespace bootstrap_key {

template <typename Lwe, typename Rlwe, typename Dcp, typename Cryptor,
          typename Holder>
  requires tlwe_concept<Lwe> && trlwe_concept<Rlwe> && decompose_concept<Dcp>
BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Dcp::l,
             Lwe::n> static generate(Cryptor& cryptor, const Holder& holder) {
  using Torus = Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t l = Dcp::l;
  static constexpr uint32_t n = Lwe::n;

  BootstrapKey<Torus, N, l, n> BK;
  for (size_t i = 0; i < n; ++i) {
    Poly<UInt, N> tmp;
    tmp[0] = static_cast<UInt>((holder.secret())[i]);
    for (size_t j = 0; j < l; ++j) {
      BK[i][j] = cryptor.encrypt(Poly<Torus, N>());
      BK[i][l + j] = cryptor.encrypt(Poly<Torus, N>());

      detail::Torus v(static_cast<detail::Torus::raw_value_type>(
                          static_cast<UInt::raw_value_type>(tmp[0])) /
                      (std::pow(B, j + 1)));
      Torus m = static_cast<Torus>(v);

      BK[i][j].a()[0] = static_cast<Torus>(BK[i][j].a()[0]) + m;
      BK[i][l + j].b()[0] = static_cast<Torus>(BK[i][l + j].b()[0]) + m;
    }
  }
  return BK;
}

}  // namespace bootstrap_key

#endif  // TFHE_BOOTSTRAP_KEY_HPP