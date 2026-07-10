// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_LWE_CRYPTOR_HPP
#define TFHE_LWE_CRYPTOR_HPP

#include <random>

#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/utility/randomize.hpp"
#include "algebra/vector.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

namespace tlwe {

template <tlwe_concept params, torus_type Torus, typename Engine>
TLWE<Torus, params::n> encrypt(std::shared_ptr<UInt::raw_value_type[]> s,
                               Engine& eng, const Torus& message) {
  TLWE<Torus, params::n> ct;
  randomize(ct.a(), eng.get());
  // Vector<UInt, params::n> secret ...
  for (uint32_t i = 0; i < ct.dimension(); ++i) {
    ct.b() += static_cast<UInt>(s[i]) * static_cast<Torus>(ct.a()[i]);
  }
  ct.b() += message;
  return ct;
}

template <tlwe_concept params, torus_type Torus>
Torus decrypt(std::shared_ptr<UInt::raw_value_type[]> s,
              const TLWE<Torus, params::n>& ct) {
  Torus pt = ct.b();
  for (uint32_t i = 0; i < ct.dimension(); ++i) {
    pt -= static_cast<UInt>(s[i]) * static_cast<Torus>(ct.a()[i]);
  }
  return pt;
}

}  // namespace tlwe

#endif  // TFHE_LWE_CRYPTOR_HPP