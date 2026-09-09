// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIPHER_BOUNDARY_HPP
#define TFHE_CIPHER_BOUNDARY_HPP

#include <cassert>
#include <cstdint>

#include "tfhe/cipher/cipher.hpp"
#include "tfhe/cipher/dial.hpp"
#include "tfhe/params.hpp"
#include "tfhe/public_runtime.hpp"
#include "tfhe/runtime.hpp"

// PublicBoundary<Lwe, PkSamples> is the public-key-only half of the
// plaintext/ciphertext boundary: lift() (Dial encode + encrypt) is all it
// can do, since a PublicRuntime never holds the secret drop() (decrypt +
// Dial decode) needs. Kept separate from Boundary below (rather than a
// third Boundary constructor) so "this party has no secret at all" is a
// fact the type system enforces -- there's no drop() to accidentally call.
//
// Resolution is a method template argument on lift(), not PublicBoundary's
// own: it names a single value's encoding, not a property of this object,
// so the same PublicBoundary lift<2>()s one value and lift<100>()s another
// without needing a separate instance per resolution.
template <typename Lwe, uint32_t PkSamples>
class PublicBoundary {
 public:
  using Torus = typename Lwe::torus_type;

  explicit PublicBoundary(PublicRuntime<Lwe, PkSamples>& pub) : pub_(&pub) {}

  template <uint32_t Resolution>
  TLWE<Torus, Lwe::n> lift(uint32_t index) const {
    return pub_->encrypt(Dial<Resolution, Torus>(index).value());
  }

 private:
  PublicRuntime<Lwe, PkSamples>* pub_;
};

// Boundary<Lwe, Rlwe, Decomp, Feature...> crosses the plaintext/ciphertext
// boundary for a party holding at least one secret -- see PublicBoundary
// above for the no-secret-at-all case. lift() (Dial encode + encrypt)
// always works; drop() (decrypt + Dial decode) works for whichever raw
// ciphertext shape(s) this Boundary was built with a secret for. Built
// with the Lwe-side secret only (e.g. a party that only ever decodes
// Lwe-shaped ciphertexts and has no lasting need for the Rlwe side's
// Runtime), it can drop() Lwe-shaped ciphertexts but not Rlwe-shaped ones.
// Built with both, it can drop() either. Which one you get is a runtime
// fact about how this was constructed, not a separate type -- the
// Rlwe-shaped drop() overload asserts it was actually given that secret.
//
// Resolution is a method template argument on lift()/drop(), not
// Boundary's own, for the same reason as PublicBoundary above -- the same
// Boundary lift<2>()s one value and drop<100>()s another, so a caller
// moving a value between resolutions (e.g. Reslot's own tests) needs only
// one Boundary over a given Runtime pair, not one per resolution.
//
// drop() takes either a raw TLWE (decrypted directly) or a Cipher (its
// current Lwe-/Rlwe-shaped state read via is_ready(), then dispatched to
// the matching raw overload) -- Resolution can't be deduced from either
// (unlike lift()'s own index argument), so it's always given explicitly:
// boundary.drop<4>(bit).
template <typename Lwe, typename Rlwe, typename Decomp, typename... Feature>
class Boundary {
 public:
  using Torus = typename Lwe::torus_type;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

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

  template <uint32_t Resolution>
  TLWE<Torus, Lwe::n> lift(uint32_t index) const {
    return lwe_runtime_->encrypt(Dial<Resolution, Torus>(index).value());
  }

  // Lwe-shaped overload -- always valid, every Boundary constructor
  // requires the Lwe-side secret.
  template <uint32_t Resolution>
  uint32_t drop(const TLWE<Torus, Lwe::n>& ct) const {
    return Dial<Resolution, Torus>(lwe_runtime_->decrypt(ct)).index();
  }

  // Valid only when built with the Rlwe-side secret.
  template <uint32_t Resolution>
  uint32_t drop(const TLWE<rTorus, N>& ct) const {
    assert(rlwe_runtime_ != nullptr);
    return Dial<Resolution, rTorus>(rlwe_runtime_->decrypt(ct)).index();
  }

  // Reads bit's current shape and dispatches to whichever raw overload
  // above applies -- the one place Boundary needs to know about Cipher.
  template <uint32_t Resolution>
  uint32_t drop(const Cipher<Lwe, Rlwe>& bit) const {
    return bit.is_ready() ? drop<Resolution>(bit.ready())
                          : drop<Resolution>(bit.pending());
  }

 private:
  Runtime<Lwe, Feature...>* lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Feature...>* rlwe_runtime_;
};

#endif  // TFHE_CIPHER_BOUNDARY_HPP
