// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIPHER_PARTY_HPP
#define TFHE_CIPHER_PARTY_HPP

#include <cstdint>
#include <optional>
#include <random>

#include "tfhe/cipher/boundary.hpp"
#include "tfhe/cipher/cipher.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// Party<Lwe, Rlwe, Decomp, Kst, Feature...> is the common single-machine
// case Boundary alone leaves the caller to assemble by hand: one party
// that holds both secrets and wants to lift()/drop() without juggling
// which Runtime does what.
//
// It owns both Runtimes (built from just a random engine); lift<Resolution>()/
// drop<Resolution>() build a Boundary<Resolution, Lwe, Rlwe, Decomp,
// Feature...> from those same two Runtimes on the spot -- reusing
// Boundary's own Dial encode/decode rather than duplicating it, and cheap
// enough to do per call since Boundary itself is just two pointers.
// Resolution is a method template argument rather than Party's own,
// because it names a single value's encoding, not a property of the
// party itself -- the same Party lift<2>()s one value and drop<100>()s
// another, same as cipher_test.cpp building several differently-resolved
// Boundarys from one pair of Runtimes.
//
// The BootstrapKey/KeySwitchKey those two Runtimes' secrets can produce
// are *not* generated in the constructor -- generate_bootstrap_key()/
// generate_key_switch_key() do that explicitly, same names as Runtime's
// own methods, so a caller that only ever needs one of the two (or
// neither, e.g. a party that just lift()s for someone else to bootstrap)
// doesn't pay for generating both up front. Once generated, Party owns
// the result -- bootstrap_key()/key_switch_key() hand back a const
// reference, the caller never stores the key itself.
//
// A party that holds only one secret, or none, still uses
// Boundary/PublicBoundary directly -- Party doesn't replace those, it's a
// convenience layer on top of them for this one common shape.
template <typename Lwe, typename Rlwe, typename Decomp, typename Kst,
          typename... Feature>
class Party {
 public:
  using Torus = typename Lwe::torus_type;
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t n = Lwe::n;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  template <typename Engine>
    requires std::uniform_random_bit_generator<Engine>
  explicit Party(Engine& eng) : lwe_runtime_(eng), rlwe_runtime_(eng) {}

  void generate_bootstrap_key() {
    bk_.emplace(
        rlwe_runtime_.template generate_bootstrap_key<Lwe, Rlwe, Decomp>(
            lwe_runtime_.secret()));
  }

  void generate_key_switch_key() {
    ksk_.emplace(
        lwe_runtime_
            .template generate_key_switch_key<ExtractedLwe<Rlwe>, Lwe, Kst>(
                rlwe_runtime_.secret()));
  }

  template <uint32_t Resolution>
  Cipher<Lwe, Rlwe> lift(uint32_t index) {
    return Cipher<Lwe, Rlwe>(boundary<Resolution>().lift(index));
  }

  template <uint32_t Resolution>
  uint32_t drop(const Cipher<Lwe, Rlwe>& bit) {
    return ::drop(boundary<Resolution>(), bit);
  }

  // Valid only after generate_bootstrap_key()/generate_key_switch_key()
  // has actually been called.
  const BootstrapKey<rTorus, N, l, n>& bootstrap_key() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return bk_.value();
  }
  const KeySwitchKey<Torus, n, t, N>& key_switch_key() const {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return ksk_.value();
  }

 private:
  template <uint32_t Resolution>
  Boundary<Resolution, Lwe, Rlwe, Decomp, Feature...> boundary() {
    return Boundary<Resolution, Lwe, Rlwe, Decomp, Feature...>(lwe_runtime_,
                                                               rlwe_runtime_);
  }

  Runtime<Lwe, Feature...> lwe_runtime_;
  Runtime<ParamsPack<Rlwe, Decomp>, Feature...> rlwe_runtime_;
  std::optional<BootstrapKey<rTorus, N, l, n>> bk_;
  std::optional<KeySwitchKey<Torus, n, t, N>> ksk_;
};

#endif  // TFHE_CIPHER_PARTY_HPP
