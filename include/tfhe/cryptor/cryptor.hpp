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

template <typename TorusT>
struct default_distribution;

template <>
struct default_distribution<ModTorus> {
  using type = std::uniform_int_distribution<ModTorus::raw_value_type>;
};

template <>
struct default_distribution<Torus> {
  using type = std::uniform_real_distribution<Torus::raw_value_type>;
};

template <typename T>
using default_distribution_t = typename default_distribution<T>::type;

namespace trlwe {

template <typename Ctx, typename Engine = std::mt19937,
          typename Dist = default_distribution_t<typename Ctx::torus_type>,
          typename params = params<Ctx>>
class Cryptor {
 public:
  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Poly<UInt>> secret, Engine& eng,
          const Dist& dist)
      : secret_(std::move(secret)), eng_(eng), dist_(dist) {
    assert(secret_->size() == params::N);
  }

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <typename TorusT>
  TRLWE<TorusT> encrypt(const Poly<TorusT>& message) {
    TRLWE<TorusT> ciphertext(params::N);
    randomize(ciphertext.a(), eng_.get(), dist_);
    ciphertext.b() = message + ((*secret_) * ciphertext.a());

    return ciphertext;
  }

  template <typename TorusT>
  Poly<TorusT> decrypt(const TRLWE<TorusT>& ciphertext) {
    Poly<TorusT> decrypted(ciphertext.a().size());

    decrypted = ciphertext.b() - ((*secret_) * ciphertext.a());

    return decrypted;
  }

 private:
  std::shared_ptr<const Poly<UInt>> secret_;
  std::reference_wrapper<Engine> eng_;
  Dist dist_;
};
}  // namespace trlwe

namespace trgsw {

template <typename Ctx, typename Engine = std::mt19937,
          typename Dist = default_distribution_t<typename Ctx::torus_type>,
          typename params = params<Ctx>>
class Cryptor {
 public:
  Cryptor() = delete;
  Cryptor(std::shared_ptr<const Poly<UInt>> secret, Engine& eng,
          const Dist& dist)
      : secret_(std::move(secret)), eng_(eng), dist_(dist) {
    assert(secret_->size() == params::N);
  }

  Cryptor(const Cryptor&) = default;
  Cryptor& operator=(const Cryptor&) = default;

  template <typename TorusT>
  TRGSW<TorusT> encrypt(const Poly<UInt>& message) {
    TRGSW<TorusT> ciphertext(params::N, params::l);
    for (size_t i = 0; i < params::l; ++i) {
      randomize(ciphertext[i].a(), eng_.get(), dist_);
      randomize(ciphertext[params::l + i].a(), eng_.get(), dist_);

      ciphertext[i].b() = (*secret_) * ciphertext[i].a();
      ciphertext[params::l + i].b() =
          ((*secret_) * ciphertext[params::l + i].a());

      Poly<TorusT> message_torus(params::N);
      Torus v = static_cast<Torus>(
          static_cast<Torus::raw_value_type>(
              static_cast<UInt::raw_value_type>(message[0])) /
          (std::pow(params::B, i + 1)));
      if constexpr (std::same_as<TorusT, ModTorus>) {
        message_torus[0] = static_cast<ModTorus>(v);
      } else if constexpr (std::same_as<TorusT, Torus>) {
        message_torus[0] = v;
      } else {
        static_assert(false, "unsupported torus_type");
      }

      ciphertext[i].a() += message_torus;
      ciphertext[params::l + i].b() += message_torus;
    }
    return ciphertext;
  }

 private:
  std::shared_ptr<const Poly<UInt>> secret_;
  std::reference_wrapper<Engine> eng_;
  Dist dist_;
};

}  // namespace trgsw

#endif