#ifndef ENCRYPT_HPP
#define ENCRYPT_HPP

#include <functional>
#include <memory>

#include "primitive/concept/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/vector.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/negacyclic_convolution.hpp"
#include "arithmetic/utility.hpp"

#include "tfhe/structure/tlwe.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

template <typename Torus>
struct default_distribution;

template <uint32_t QBit>
struct default_distribution<ModTorus<QBit>> {
  using type =
      std::uniform_int_distribution<typename ModTorus<QBit>::raw_value_type>;
};

template <typename T>
using default_distribution_t = typename default_distribution<T>::type;

namespace tlwe {
template <tlwe_concept params, typename Engine = std::mt19937>
class Cryptor {
 public:
  template <torus_type Torus>
  using Ciphertext = TLWE<Torus, params::n>;

  template <torus_type Torus>
  using Plaintext = Torus;

  using Secret = Vector<UInt, params::n>;

  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Secret> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {
    assert(secret_->size() == params::n);
  }

  template <torus_type Torus>
  Ciphertext<Torus> encrypt(const Plaintext<Torus>& message) {
    auto dist =
        default_distribution_t<Torus>(Torus::raw_min(), Torus::raw_max());
    Ciphertext<Torus> ct;
    randomize(ct.a(), eng_.get(), dist);
    for (size_t i = 0; i < ct.dimension(); ++i) {
      ct.b() +=
          static_cast<UInt>((*secret_)[i]) * static_cast<Torus>(ct.a()[i]);
    }
    ct.b() += message;
    return ct;
  }

 private:
  std::shared_ptr<const Secret> secret_;
  std::reference_wrapper<Engine> eng_;
};
}  // namespace tlwe

namespace trlwe {

template <trlwe_concept params, typename Engine = std::mt19937>
class Cryptor {
 public:
  template <torus_type Torus>
  using Ciphertext = TRLWE<Torus, params::N>;

  template <torus_type Torus>
  using Plaintext = Poly<Torus, params::N>;

  using Secret = Poly<UInt, params::N>;

  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Secret> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {
    assert(secret_->size() == params::N);
  }

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <torus_type Torus>
  Ciphertext<Torus> encrypt(const Plaintext<Torus>& message) {
    auto dist =
        default_distribution_t<Torus>(Torus::raw_min(), Torus::raw_max());
    Ciphertext<Torus> ct;
    randomize(ct.a(), eng_.get(), dist);
    ct.b() = message + negacyclic_convolution((*secret_), ct.a());
    ct.update_bound(0.0);  // TOOO: use params
    return ct;
  }

  template <torus_type Torus>
  Plaintext<Torus> decrypt(const Ciphertext<Torus>& ciphertext) {
    return ciphertext.b() - negacyclic_convolution((*secret_), ciphertext.a());
  }

 private:
  std::shared_ptr<const Secret> secret_;
  std::reference_wrapper<Engine> eng_;
};
}  // namespace trlwe

namespace trgsw {

template <trgsw_concept params, typename Engine = std::mt19937>
class Cryptor {
 public:
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;

  template <torus_type Torus>
  using Ciphertext = TRGSW<Torus, N, l>;

  using Plaintext = Poly<UInt, N>;

  using Secret = Poly<UInt, N>;

  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Secret> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {
    assert(secret_->size() == N);
  }

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <torus_type Torus>
  Ciphertext<Torus> encrypt(const Plaintext& message) {
    Ciphertext<Torus> ct;
    auto dist =
        default_distribution_t<Torus>(Torus::raw_min(), Torus::raw_max());
    for (size_t i = 0; i < l; ++i) {
      randomize(ct[i].a(), eng_.get(), dist);
      randomize(ct[l + i].a(), eng_.get(), dist);

      ct[i].b() = negacyclic_convolution(*secret_, ct[i].a());
      ct[l + i].b() = negacyclic_convolution(*secret_, ct[l + i].a());

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
  std::shared_ptr<const Secret> secret_;
  std::reference_wrapper<Engine> eng_;
};

}  // namespace trgsw

#endif