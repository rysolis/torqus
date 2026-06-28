#ifndef TFHE_KEYRING_HPP
#define TFHE_KEYRING_HPP

#include <cstdint>
#include <memory>
#include <random>

#include "primitive/uint.hpp"

#include "algebra/poly.hpp"

#include "tfhe/cryptor/cryptor.hpp"
#include "tfhe/params.hpp"

template <typename params, typename = void>
class KeyRing {};

template <typename params>
class KeyRing<params, std::enable_if_t<tlwe_concept<params>>> {
 public:
  static constexpr uint32_t n = params::n;

  template <typename Engine>
  explicit KeyRing(Engine& eng) {
    std::uniform_int_distribution<UInt::raw_value_type> dist{0, 1};
    this->secret_ = std::make_shared<const Vector<UInt, n>>(
        [&eng, &dist] { return static_cast<UInt>(dist(eng)); });
  }

  template <typename Engine>
  auto tlwe_cryptor(Engine& eng) {
    return std::make_unique<tlwe::Cryptor<params>>(this->secret_, eng);
  }

  const Vector<UInt, n>& secret() const noexcept { return *secret_; }

 private:
  std::shared_ptr<const Vector<UInt, n>> secret_;
};

template <typename params>
class KeyRing<
    params, std::enable_if_t<trgsw_concept<params> || trlwe_concept<params>>> {
 public:
  static constexpr uint32_t N = params::N;

  template <typename Engine>
  explicit KeyRing(Engine& eng) {
    std::uniform_int_distribution<UInt::raw_value_type> dist{0, 1};
    this->secret_ = std::make_shared<const Poly<UInt, N>>(
        [&eng, &dist]() { return static_cast<UInt>(dist(eng)); });
  }

  template <typename Engine>
  auto trlwe_cryptor(Engine& eng) {
    return std::make_unique<trlwe::Cryptor<params>>(this->secret_, eng);
  }

  template <typename Engine>
  auto trgsw_cryptor(Engine& eng) {
    return std::make_unique<trgsw::Cryptor<params>>(this->secret_, eng);
  }

  const Poly<UInt, N>& secre() const noexcept { return *secret_; }

 private:
  std::shared_ptr<const Poly<UInt, N>> secret_;
};

#endif