// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_LWE_CRYPTOR_HPP
#define TFHE_LWE_CRYPTOR_HPP

#include <random>

#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/vector.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"

namespace tlwe::detail {

template <typename Torus>
struct default_distribution;

template <uint32_t QBit>
struct default_distribution<ModTorus<QBit>> {
  using type =
      std::uniform_int_distribution<typename ModTorus<QBit>::raw_value_type>;
};

template <typename T>
using default_distribution_t = typename default_distribution<T>::type;

}  // namespace tlwe::detail

namespace tlwe {

template <tlwe_concept params, typename Engine = std::mt19937>
class Cryptor {
 public:
  using params_type = params;

  template <torus_type Torus>
  using Ciphertext = TLWE<Torus, params::n>;

  template <torus_type Torus>
  using Plaintext = Torus;

  using Secret = Vector<UInt, params::n>;

  Cryptor(std::shared_ptr<UInt::raw_value_type[]> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {}

  template <torus_type Torus>
  Ciphertext<Torus> encrypt(const Plaintext<Torus>& message) {
    auto dist = tlwe::detail::default_distribution_t<Torus>(Torus::raw_min(),
                                                            Torus::raw_max());
    Ciphertext<Torus> ct;
    randomize(ct.a(), eng_.get(), dist);
    // Vector<UInt, params::n> secret ...
    for (uint32_t i = 0; i < ct.dimension(); ++i) {
      ct.b() += static_cast<UInt>(secret_[i]) * static_cast<Torus>(ct.a()[i]);
    }
    ct.b() += message;
    return ct;
  }

  template <torus_type Torus>
  Plaintext<Torus> decrypt(const Ciphertext<Torus>& ciphertext) {
    Plaintext<Torus> pt = ciphertext.b();
    for (uint32_t i = 0; i < ciphertext.dimension(); ++i) {
      pt -=
          static_cast<UInt>(secret_[i]) * static_cast<Torus>(ciphertext.a()[i]);
    }
    return pt;
  }

 private:
  std::shared_ptr<UInt::raw_value_type[]> secret_;
  std::reference_wrapper<Engine> eng_;
};
}  // namespace tlwe

#endif  // TFHE_LWE_CRYPTOR_HPP