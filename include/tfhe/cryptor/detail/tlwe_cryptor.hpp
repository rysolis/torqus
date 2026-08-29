// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TLWE_CRYPTOR_HPP
#define TFHE_TLWE_CRYPTOR_HPP

#include <random>

#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/utility/randomize.hpp"
#include "algebra/vector.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

namespace tlwe {

template <tlwe_concept Params, torus_concept Torus, typename Engine>
TLWE<Torus, Params::n> encrypt(
    const std::shared_ptr<typename UInt::raw_value_type[]>& secret, Engine& eng,
    const Torus& message) {
  TLWE<Torus, Params::n> ct;
  randomize(ct.a(), eng.get());
  // Vector<UInt, Params::n> secret ...
  for (uint32_t i = 0; i < ct.dimension(); ++i) {
    ct.b() += static_cast<UInt>(secret[i]) * static_cast<Torus>(ct.a()[i]);
  }
  ct.b() += message;
  ct.b() += gaussian_noise<Torus>(eng.get(), alpha_of<Params>::value);
  return ct;
}

template <tlwe_concept Params, torus_concept Torus>
Torus decrypt(const std::shared_ptr<UInt::raw_value_type[]>& secret,
              const TLWE<Torus, Params::n>& ct) {
  Torus pt = ct.b();
  for (uint32_t i = 0; i < ct.dimension(); ++i) {
    pt -= static_cast<UInt>(secret[i]) * static_cast<Torus>(ct.a()[i]);
  }
  return pt;
}

}  // namespace tlwe

#endif  // TFHE_TLWE_CRYPTOR_HPP