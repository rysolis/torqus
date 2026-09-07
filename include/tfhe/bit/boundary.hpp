// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BOUNDARY_HPP
#define TFHE_BOUNDARY_HPP

#include <cassert>
#include <cstdint>

#include "tfhe/bit/bit.hpp"
#include "tfhe/bit/dial.hpp"
#include "tfhe/params.hpp"
#include "tfhe/public_runtime.hpp"
#include "tfhe/runtime.hpp"

// PublicBoundary<Resolution, Lwe, PkSamples> is the public-key-only half of
// the plaintext/ciphertext boundary: lift() (Dial encode + encrypt) is all
// it can do, since a PublicRuntime never holds the secret drop() (decrypt +
// Dial decode) needs. Kept separate from Boundary below (rather than a
// third Boundary constructor) so "this party has no secret at all" is a
// fact the type system enforces -- there's no drop() to accidentally call.
template <uint32_t Resolution, typename Lwe, uint32_t PkSamples>
class PublicBoundary {
 public:
  using Torus = typename Lwe::torus_type;
  using Plain = Dial<Resolution, Torus>;

  explicit PublicBoundary(PublicRuntime<Lwe, PkSamples>& pub) : pub_(&pub) {}

  TLWE<Torus, Lwe::n> lift(uint32_t index) const {
    return pub_->encrypt(Plain(index).value());
  }

 private:
  PublicRuntime<Lwe, PkSamples>* pub_;
};

// Boundary<Resolution, Lwe, Rlwe, Decomp, Feature...> crosses the
// plaintext/ciphertext boundary for a party holding at least one secret --
// see PublicBoundary above for the no-secret-at-all case. lift() (Dial
// encode + encrypt) always works; drop() (decrypt + Dial decode) works for
// whichever raw ciphertext shape(s) this Boundary was built with a secret
// for. Built with the Lwe-side secret only (e.g. a party that only ever
// decodes Lwe-shaped ciphertexts and has no lasting need for the Rlwe
// side's Runtime), it can drop() Lwe-shaped ciphertexts but not Rlwe-shaped
// ones. Built with both, it can drop() either. Which one you get is a
// runtime fact about how this was constructed, not a separate type -- the
// Rlwe-shaped drop() overload asserts it was actually given that secret.
//
// Boundary itself only knows about raw ciphertexts -- it has no notion of
// Bit's own Lwe-shaped/Rlwe-shaped distinction. The free drop(boundary,
// bit) overload below bridges a Bit to whichever raw drop() applies.
template <uint32_t Resolution, typename Lwe, typename Rlwe, typename Decomp,
          typename... Feature>
class Boundary {
 public:
  using Torus = typename Lwe::torus_type;
  using Plain = Dial<Resolution, Torus>;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  using RPlain = Dial<Resolution, rTorus>;

  // Holds the secret on the Lwe side only -- drop() works for Lwe-shaped
  // ciphertexts, not Rlwe-shaped ones. For a party (e.g. a tally decoder)
  // that never needs to decode a gate's own still-pending output and would
  // otherwise have to keep the Rlwe-side Runtime alive as a member just to
  // satisfy the two-Runtime constructor below.
  explicit Boundary(Runtime<Lwe, Feature...>& lwe_runtime)
      : lwe_runtime_(&lwe_runtime), rlwe_runtime_(nullptr) {}

  // Holds the secret on both sides: lift() and drop() both work for
  // either ciphertext shape.
  Boundary(Runtime<Lwe, Feature...>& lwe_runtime,
           Runtime<ParamsPack<Rlwe, Decomp>, Feature...>& rlwe_runtime)
      : lwe_runtime_(&lwe_runtime), rlwe_runtime_(&rlwe_runtime) {}

  TLWE<Torus, Lwe::n> lift(uint32_t index) const {
    return lwe_runtime_->encrypt(Plain(index).value());
  }

  // Lwe-shaped overload -- always valid, every Boundary constructor
  // requires the Lwe-side secret.
  uint32_t drop(const TLWE<Torus, Lwe::n>& ct) const {
    return Plain(lwe_runtime_->decrypt(ct)).index();
  }

  // Valid only when built with the Rlwe-side secret.
  uint32_t drop(const TLWE<rTorus, N>& ct) const {
    assert(rlwe_runtime_ != nullptr);
    return RPlain(rlwe_runtime_->decrypt(ct)).index();
  }

 private:
  Runtime<Lwe, Feature...>* lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Feature...>* rlwe_runtime_;
};

// Bridges a Bit's current shape (Lwe- or Rlwe-shaped) to the matching
// Boundary::drop() overload -- Boundary itself has no dependency on Bit,
// so this is the one place that needs to know both.
template <uint32_t Resolution, typename Lwe, typename Rlwe, typename Decomp,
          typename... Feature>
uint32_t drop(
    const Boundary<Resolution, Lwe, Rlwe, Decomp, Feature...>& boundary,
    const Bit<Lwe, Rlwe>& bit) {
  return bit.is_ready() ? boundary.drop(bit.ready())
                        : boundary.drop(bit.pending());
}

#endif  // TFHE_BOUNDARY_HPP
