// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_GLWE_CRYPTOR_HPP
#define TFHE_GLWE_CRYPTOR_HPP

#include <functional>
#include <memory>
#include <random>

#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/utility/randomize.hpp"
#include "algebra/vector.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace trlwe {

template <trlwe_concept params, torus_type Torus, typename Engine>
TRLWE<Torus, params::N> encrypt(std::shared_ptr<UInt::raw_value_type[]> s,
                                Engine& eng, const Poly<Torus, params::N>& pt) {
  constexpr uint32_t N = params::N;

  TRLWE<Torus, N> ct;
  randomize(ct.a(), eng.get());
  Poly<UInt, N> secret(s.get(), s.get() + N);
  ct.b() = pt + negacyclic_convolution(secret, ct.a());
  return ct;
}

template <trlwe_concept params, torus_type Torus>
Poly<Torus, params::N> decrypt(std::shared_ptr<UInt::raw_value_type[]> s,
                               const TRLWE<Torus, params::N>& ct) {
  constexpr uint32_t N = params::N;

  Poly<UInt, N> secret(s.get(), s.get() + N);
  return ct.b() - negacyclic_convolution(secret, ct.a());
}

}  // namespace trlwe

#endif  // TFHE_GLWE_CRYPTOR_HPP