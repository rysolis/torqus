// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TRLWE_CRYPTOR_HPP
#define TFHE_TRLWE_CRYPTOR_HPP

#include <functional>
#include <memory>
#include <random>

#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/utility/randomize.hpp"
#include "algebra/vector.hpp"

#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace trlwe {

template <trlwe_concept Params, torus_concept Torus, typename Engine>
TRLWE<Torus, Params::N> encrypt(std::shared_ptr<UInt::raw_value_type[]> secret,
                                Engine& eng, const Poly<Torus, Params::N>& pt) {
  constexpr uint32_t N = Params::N;

  TRLWE<Torus, N> ct;
  randomize(ct.a(), eng.get());
  Poly<UInt, N> secret_poly(secret.get(), secret.get() + N);
  ct.b() = pt + negacyclic_convolution(secret_poly, ct.a());
  return ct;
}

template <trlwe_concept Params, torus_concept Torus>
Poly<Torus, Params::N> decrypt(std::shared_ptr<UInt::raw_value_type[]> secret,
                               const TRLWE<Torus, Params::N>& ct) {
  constexpr uint32_t N = Params::N;

  Poly<UInt, N> secret_poly(secret.get(), secret.get() + N);
  return ct.b() - negacyclic_convolution(secret_poly, ct.a());
}

template <trlwe_concept Params, torus_concept Torus>
Torus decrypt(std::shared_ptr<UInt::raw_value_type[]> secret,
              const TLWE<Torus, Params::N>& ct) {
  Torus pt = ct.b();
  for (uint32_t i = 0; i < ct.dimension(); ++i) {
    pt -= static_cast<UInt>(secret[i]) * static_cast<Torus>(ct.a()[i]);
  }
  return pt;
}

}  // namespace trlwe

#endif  // TFHE_TRLWE_CRYPTOR_HPP