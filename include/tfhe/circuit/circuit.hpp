// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_HPP
#define TFHE_CIRCUIT_HPP

#include <cstdint>
#include <utility>

#include "tfhe/bit/cipher.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"

// BootstrapKeyHolder<Lwe, Rlwe, Decomp> owns a BootstrapKey and manages its
// lifetime -- the one place that key actually lives. tfhe::circuit::Reslot
// and BinaryExpansion each take the raw BootstrapKey itself (a caller
// holding a BootstrapKeyHolder passes holder.bk()), not the holder --
// those don't need to know the holder concept exists; the holder's only
// job is giving the key a name and a stable address several of them can
// point at, with no one of them privileged as "the real owner." The
// referenced key must outlive anything built from it.
template <typename Lwe, typename Rlwe, typename Decomp>
class BootstrapKeyHolder {
 public:
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  static constexpr uint32_t l = Decomp::l;

  BootstrapKeyHolder() = default;
  explicit BootstrapKeyHolder(BootstrapKey<rTorus, N, l, n> bk)
      : bk_(std::move(bk)) {}

  const BootstrapKey<rTorus, N, l, n>& bk() const { return bk_; }

 private:
  BootstrapKey<rTorus, N, l, n> bk_;
};

// KeySwitchKeyHolder<Lwe, Rlwe, Kst> is BootstrapKeyHolder's counterpart
// for the KeySwitchKey Relay below needs.
template <typename Lwe, typename Rlwe, typename Kst>
class KeySwitchKeyHolder {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t t = Kst::t;

  KeySwitchKeyHolder() = default;
  explicit KeySwitchKeyHolder(KeySwitchKey<Torus, n, t, N> ksk)
      : ksk_(std::move(ksk)) {}

  const KeySwitchKey<Torus, n, t, N>& ksk() const { return ksk_; }

 private:
  KeySwitchKey<Torus, n, t, N> ksk_;
};

// Relay<Lwe, Rlwe, Kst> holds a pointer to the raw KeySwitchKey needed to
// materialize a Cipher -- converting a gate's Rlwe-shaped result back
// down to Lwe-shaped so it can feed into another gate call. Takes the key
// itself, not a KeySwitchKeyHolder -- a caller holding one passes
// holder.ksk() straight through; Relay has no need to know the holder
// concept exists.
//
// The referenced key must outlive this Relay.
template <typename Lwe, typename Rlwe, typename Kst>
class Relay {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t t = Kst::t;

  Relay() = default;
  explicit Relay(const KeySwitchKey<Torus, n, t, N>& ksk) : ksk_(&ksk) {}

  // Converts `bit` in place to Lwe-shaped -- a no-op if already
  // bit.is_ready().
  void materialize(Cipher<Lwe, Rlwe>& bit) const {
    bit.template materialize<Kst>(*ksk_);
  }

 private:
  const KeySwitchKey<Torus, n, t, N>* ksk_;
};

#endif  // TFHE_CIRCUIT_HPP
