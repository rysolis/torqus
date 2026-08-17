// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TRGSW_CRYPTOR_HPP
#define TFHE_TRGSW_CRYPTOR_HPP

#include <memory>

#include "primitive/concept/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/utility/randomize.hpp"

#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"

namespace trgsw {

template <typename Params, typename Engine>
  requires trlwe_concept<Params> && decompose_concept<Params>
TRGSW<typename Params::torus_type, Params::N, Params::l> encrypt(
    std::shared_ptr<UInt::raw_value_type[]> secret, Engine& eng,
    const Poly<UInt, Params::N>& pt) {
  using Torus = typename Params::torus_type;
  constexpr uint32_t N = Params::N;
  constexpr uint32_t B = Params::B;
  constexpr uint32_t Bbit = std::bit_width(B - 1);
  constexpr uint32_t l = Params::l;

  TRGSW<Torus, N, l> ct;
  const Poly<UInt, N> secret_poly(secret.get(), secret.get() + N);

  for (size_t j = 0; j < l; ++j) {
    randomize(ct[j].a(), eng.get());
    randomize(ct[l + j].a(), eng.get());

    ct[j].b() = negacyclic_convolution(secret_poly, ct[j].a());
    ct[l + j].b() = negacyclic_convolution(secret_poly, ct[l + j].a());

    Torus m(static_cast<UInt::raw_value_type>(pt[0]), 1u << (Bbit * (j + 1)));

    ct[j].a()[0] = static_cast<Torus>(ct[j].a()[0]) + m;
    ct[l + j].b()[0] = static_cast<Torus>(ct[l + j].b()[0]) + m;
  }
  return ct;
}

}  // namespace trgsw

#endif