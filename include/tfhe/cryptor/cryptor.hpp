#ifndef ENCRYPT_HPP
#define ENCRYPT_HPP

#include <functional>
#include <memory>

#include "arithmetic/multiplication.hpp"
#include "arithmetic/utility.hpp"
#include "primitive/uint.hpp"
#include "tfhe/adapter/adapter.hpp"
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

namespace trlwe {

template <typename Ctx, typename Engine = std::mt19937,
          typename params = params<Ctx>>
class Cryptor {
 public:
  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Poly<UInt>> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {
    assert(secret_->size() == params::N);
  }

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <TorusType Torus>
  TRLWE<Torus> encrypt(const Poly<Torus>& message) {
    auto dist =
        default_distribution_t<Torus>(Torus::raw_min(), Torus::raw_max());
    TRLWE<Torus> ciphertext(params::N);
    randomize(ciphertext.a(), eng_.get(), dist);
    ciphertext.b() = message + ((*secret_) * ciphertext.a());

    return ciphertext;
  }

  template <TorusType Torus>
  Poly<Torus> decrypt(const TRLWE<Torus>& ciphertext) {
    Poly<Torus> decrypted(ciphertext.a().size());

    decrypted = ciphertext.b() - ((*secret_) * ciphertext.a());

    return decrypted;
  }

 private:
  std::shared_ptr<const Poly<UInt>> secret_;
  std::reference_wrapper<Engine> eng_;
};
}  // namespace trlwe

namespace trgsw {

template <typename Ctx, typename Engine = std::mt19937,
          typename params = params<Ctx>>
class Cryptor {
 public:
  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Poly<UInt>> secret, Engine& eng)
      : secret_(std::move(secret)), eng_(eng) {
    assert(secret_->size() == params::N);
  }

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <TorusType Torus>
  TRGSW<Torus> encrypt(const Poly<UInt>& message) {
    TRGSW<Torus> ciphertext(params::N, params::l);
    auto dist =
        default_distribution_t<Torus>(Torus::raw_min(), Torus::raw_max());
    for (size_t i = 0; i < params::l; ++i) {
      randomize(ciphertext[i].a(), eng_.get(), dist);
      randomize(ciphertext[params::l + i].a(), eng_.get(), dist);

      ciphertext[i].b() = (*secret_) * ciphertext[i].a();
      ciphertext[params::l + i].b() =
          ((*secret_) * ciphertext[params::l + i].a());

      Poly<Torus> message_torus(params::N);
      detail::Torus v(static_cast<detail::Torus::raw_value_type>(
                          static_cast<UInt::raw_value_type>(message[0])) /
                      (std::pow(params::B, i + 1)));
      message_torus[0] = static_cast<Torus>(v);

      ciphertext[i].a() += message_torus;
      ciphertext[params::l + i].b() += message_torus;
    }
    return ciphertext;
  }

 private:
  std::shared_ptr<const Poly<UInt>> secret_;
  std::reference_wrapper<Engine> eng_;
};

}  // namespace trgsw

#endif