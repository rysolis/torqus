// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BOUNDARY_HPP
#define TFHE_BOUNDARY_HPP

#include <cassert>
#include <cstdint>
#include <functional>

#include "tfhe/bit.hpp"
#include "tfhe/dial.hpp"
#include "tfhe/params.hpp"
#include "tfhe/public_runtime.hpp"
#include "tfhe/runtime.hpp"

// Boundary<Resolution, Lwe, Rlwe, Decomp, Feature...> crosses the
// plaintext/ciphertext boundary -- lift() (Dial encode + encrypt) always
// works, drop() (decrypt + Dial decode) only if this Boundary was built
// with the actual secret (see has_secret()): built from just a
// PublicRuntime, it can lift but has no secret to drop() with; built from
// both Runtimes (the party holding the secret), it can do both. Which one
// you get is a runtime fact about how this was constructed, not a
// separate type -- drop() asserts has_secret().
template <uint32_t Resolution, typename Lwe, typename Rlwe, typename Decomp,
          typename... Feature>
class Boundary {
 public:
  using Torus = typename Lwe::torus_type;
  using Plain = Dial<Resolution, Torus>;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  using RPlain = Dial<Resolution, rTorus>;

  // Public-key only: lift() works, drop() does not.
  template <uint32_t PkSamples>
  explicit Boundary(PublicRuntime<Lwe, PkSamples>& pub)
      : lift_([&pub](const Torus& v) -> TLWE<Torus, Lwe::n> {
          return pub.encrypt(v);
        }),
        rlwe_runtime_(nullptr) {}

  // Holds the actual secret on both sides: lift() and drop() both work.
  Boundary(Runtime<Lwe, Feature...>& lwe_runtime,
           Runtime<ParamsPack<Rlwe, Decomp>, Feature...>& rlwe_runtime)
      : lift_([&lwe_runtime](const Torus& v) -> TLWE<Torus, Lwe::n> {
          return lwe_runtime.encrypt(v);
        }),
        rlwe_runtime_(&rlwe_runtime) {}

  // True once constructed with the secret -- safe to call drop() only
  // when this is true.
  bool has_secret() const { return rlwe_runtime_ != nullptr; }

  Bit<Lwe, Rlwe> lift(uint32_t index) const {
    return lift_(Plain(index).value());
  }

  // Valid only when has_secret().
  uint32_t drop(const Bit<Lwe, Rlwe>& bit) const {
    assert(has_secret());
    return RPlain(rlwe_runtime_->decrypt(bit.pending())).index();
  }

 private:
  std::function<TLWE<Torus, Lwe::n>(const Torus&)> lift_;
  Runtime<ParamsPack<Rlwe, Decomp>, Feature...>* rlwe_runtime_;
};

#endif  // TFHE_BOUNDARY_HPP
