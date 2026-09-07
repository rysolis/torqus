// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BOUNDARY_HPP
#define TFHE_BOUNDARY_HPP

#include <cassert>
#include <cstdint>
#include <functional>

#include "tfhe/bit/bit.hpp"
#include "tfhe/bit/dial.hpp"
#include "tfhe/params.hpp"
#include "tfhe/public_runtime.hpp"
#include "tfhe/runtime.hpp"

// Boundary<Resolution, Lwe, Rlwe, Decomp, Feature...> crosses the
// plaintext/ciphertext boundary -- lift() (Dial encode + encrypt) always
// works, drop() (decrypt + Dial decode) only for whichever ciphertext
// shape(s) this Boundary was built with a secret for (see has_secret()).
// Built from just a PublicRuntime, it can lift but drop() nothing. Built
// with the Lwe-side secret only (e.g. a party that only ever decodes
// Lwe-shaped ciphertexts and has no lasting need for the Rlwe side's
// Runtime), it can drop() Lwe-shaped ciphertexts but not Rlwe-shaped ones.
// Built with both, it can drop() either. Which one you get is a runtime
// fact about how this was constructed, not a separate type -- each drop()
// overload asserts it has the specific secret it needs.
//
// drop() has a raw-ciphertext overload for each shape (Lwe- and
// Rlwe-shaped) plus a Bit overload that dispatches to whichever applies
// via Bit::is_ready() -- so a caller with either a Bit or a raw ciphertext
// can always just call drop() and get the right decode.
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
        lwe_runtime_(nullptr),
        rlwe_runtime_(nullptr) {}

  // Holds the secret on the Lwe side only -- drop() works for Lwe-shaped
  // ciphertexts (and a Bit that happens to be Lwe-shaped), not Rlwe-shaped
  // ones. For a party (e.g. a tally decoder) that never needs to decode a
  // gate's own still-pending output and would otherwise have to keep the
  // Rlwe-side Runtime alive as a member just to satisfy the two-Runtime
  // constructor below.
  explicit Boundary(Runtime<Lwe, Feature...>& lwe_runtime)
      : lift_([&lwe_runtime](const Torus& v) -> TLWE<Torus, Lwe::n> {
          return lwe_runtime.encrypt(v);
        }),
        lwe_runtime_(&lwe_runtime),
        rlwe_runtime_(nullptr) {}

  // Holds the secret on both sides: lift() and drop() both work for
  // either ciphertext shape.
  Boundary(Runtime<Lwe, Feature...>& lwe_runtime,
           Runtime<ParamsPack<Rlwe, Decomp>, Feature...>& rlwe_runtime)
      : lift_([&lwe_runtime](const Torus& v) -> TLWE<Torus, Lwe::n> {
          return lwe_runtime.encrypt(v);
        }),
        lwe_runtime_(&lwe_runtime),
        rlwe_runtime_(&rlwe_runtime) {}

  // True once constructed with at least one secret -- which drop()
  // overload(s) are actually safe to call still depends on which
  // constructor built this (each overload asserts its own requirement).
  bool has_secret() const {
    return lwe_runtime_ != nullptr || rlwe_runtime_ != nullptr;
  }

  Bit<Lwe, Rlwe> lift(uint32_t index) const {
    return lift_(Plain(index).value());
  }

  // Valid only when built with the Lwe-side secret.
  uint32_t drop(const TLWE<Torus, Lwe::n>& ct) const {
    assert(lwe_runtime_ != nullptr);
    return Plain(lwe_runtime_->decrypt(ct)).index();
  }

  // Valid only when built with the Rlwe-side secret.
  uint32_t drop(const TLWE<rTorus, N>& ct) const {
    assert(rlwe_runtime_ != nullptr);
    return RPlain(rlwe_runtime_->decrypt(ct)).index();
  }

  // Dispatches to whichever raw overload above matches bit's current
  // shape -- valid only when built with the secret that shape needs.
  uint32_t drop(const Bit<Lwe, Rlwe>& bit) const {
    return bit.is_ready() ? drop(bit.ready()) : drop(bit.pending());
  }

 private:
  std::function<TLWE<Torus, Lwe::n>(const Torus&)> lift_;
  Runtime<Lwe, Feature...>* lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Feature...>* rlwe_runtime_;
};

#endif  // TFHE_BOUNDARY_HPP
