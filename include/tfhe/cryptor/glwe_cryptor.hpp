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

#include "algebra/vector.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/negacyclic_convolution.hpp"
#include "arithmetic/utility.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace trlwe::detail {

template <typename Torus>
struct default_distribution;

template <uint32_t QBit>
struct default_distribution<ModTorus<QBit>> {
  using type =
      std::uniform_int_distribution<typename ModTorus<QBit>::raw_value_type>;
};

template <typename T>
using default_distribution_t = typename default_distribution<T>::type;

}  // namespace trlwe::detail

namespace trlwe {

template <trlwe_concept params, typename Engine = std::mt19937>
class Cryptor {
 public:
  using params_type = params;
  static constexpr uint32_t N = params::N;

  template <torus_type Torus>
  using Ciphertext = TRLWE<Torus, N>;

  template <torus_type Torus>
  using Plaintext = Poly<Torus, N>;

  using Secret = Poly<UInt, N>;

  Cryptor(std::shared_ptr<UInt::raw_value_type[]> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {}

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <torus_type Torus>
  Ciphertext<Torus> encrypt(const Plaintext<Torus>& message) {
    auto dist = trlwe::detail::default_distribution_t<Torus>(Torus::raw_min(),
                                                             Torus::raw_max());
    Ciphertext<Torus> ct;
    randomize(ct.a(), eng_.get(), dist);
    Poly<UInt, N> secret(secret_.get(), secret_.get() + N);
    ct.b() = message + negacyclic_convolution(secret, ct.a());
    return ct;
  }

  template <torus_type Torus>
  Plaintext<Torus> decrypt(const Ciphertext<Torus>& ciphertext) {
    Poly<UInt, N> secret(secret_.get(), secret_.get() + N);
    return ciphertext.b() - negacyclic_convolution(secret, ciphertext.a());
  }

 private:
  std::shared_ptr<UInt::raw_value_type[]> secret_;
  std::reference_wrapper<Engine> eng_;
};
}  // namespace trlwe

namespace trgsw {

template <trgsw_concept params, typename Engine = std::mt19937>
class Cryptor {
 public:
  using params_type = params;
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  template <torus_type Torus>
  using Ciphertext = TRGSW<Torus, N, l>;

  using Plaintext = Poly<UInt, N>;

  using Secret = Poly<UInt, N>;

  Cryptor(std::shared_ptr<UInt::raw_value_type[]> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {}

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <torus_type Torus>
  Ciphertext<Torus> encrypt(const Plaintext& message) {
    Ciphertext<Torus> ct;
    auto dist = trlwe::detail::default_distribution_t<Torus>(Torus::raw_min(),
                                                             Torus::raw_max());
    Poly<UInt, N> secret(secret_.get(), secret_.get() + N);
    for (size_t i = 0; i < l; ++i) {
      randomize(ct[i].a(), eng_.get(), dist);
      randomize(ct[l + i].a(), eng_.get(), dist);

      ct[i].b() = negacyclic_convolution(secret, ct[i].a());
      ct[l + i].b() = negacyclic_convolution(secret, ct[l + i].a());

      detail::Torus v(static_cast<detail::Torus::raw_value_type>(
                          static_cast<UInt::raw_value_type>(message[0])) /
                      (std::pow(params::B, i + 1)));
      Torus m = static_cast<Torus>(v);

      ct[i].a()[0] = static_cast<Torus>(ct[i].a()[0]) + m;
      ct[l + i].b()[0] = static_cast<Torus>(ct[l + i].b()[0]) + m;
    }
    return ct;
  }

 private:
  std::shared_ptr<UInt::raw_value_type[]> secret_;
  std::reference_wrapper<Engine> eng_;
};

}  // namespace trgsw

#endif  // TFHE_GLWE_CRYPTOR_HPP