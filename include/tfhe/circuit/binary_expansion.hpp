// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_BINARY_EXPANSION_HPP
#define TFHE_BINARY_EXPANSION_HPP

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "tfhe/cipher/cipher.hpp"
#include "tfhe/circuit/relay.hpp"
#include "tfhe/gate/hom_and.hpp"
#include "tfhe/gate/hom_and_not.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

// H is the size of the one-hot output vector this expansion produces from
// k = ceil(log2(H)) Lwe-shaped input bit-ciphertexts. Each slot chains
// HomAnd/HomAndNot through Cipher<Lwe, Rlwe>, materializing between steps
// (via this instance's own Relay); only the last step per slot stays
// Rlwe-shaped, so the whole thing has the same Lwe-in/Rlwe-out shape a
// single gate does. Output is Cipher, not raw TLWE, so chaining this
// circuit's result into another gate call needs no manual rewrapping
// (Vector<T,Size> itself can't hold Cipher -- it stores element types as a
// flat raw_value_type buffer, which Cipher's std::variant state doesn't fit;
// TLWE input is std::vector for the same reason -- neither is the
// numeric-primitive Vector<T,Size> is built for).
//
// Backend mirrors HomAnd/HomAndNot's own Backend parameter (see
// hom_and.hpp) -- this is what actually lets the H*k Bootstrap+KeySwitch
// calls in exec_slot_impl below run against a non-default (e.g. hardware)
// backend, simply by instantiating this with one.
//
// FuseKeySwitch (false by default) picks which of Backend's two exec_impl
// overloads exec_slot_impl calls -- true calls its Kst-templated one,
// landing already Lwe-shaped, so relay_ has nothing left to do; false
// calls the plain one and relay_ does the KeySwitch as usual. An explicit
// choice, not detected from Backend -- Backend must actually provide the
// Kst-templated overload when FuseKeySwitch is true.
//
// Takes the BootstrapKey/KeySwitchKey directly -- a caller just passes its
// own key values straight through. Holds a pointer to the BootstrapKey
// (not a copy) and its own Relay built from the KeySwitchKey -- the
// referenced keys must outlive this BinaryExpansion.
namespace tfhe::circuit {

template <uint32_t H, typename Lwe, typename Rlwe, typename Decomp,
          typename Kst,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap,
          bool FuseKeySwitch = false>
class BinaryExpansion {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t t = Kst::t;

  static constexpr uint32_t k = std::bit_width(H - 1);

  BinaryExpansion() = default;
  BinaryExpansion(const BootstrapKey<rTorus, N, l, n>& bk,
                  const KeySwitchKey<Torus, n, t, N>& ksk)
      : bk_(&bk), relay_(ksk) {}

  // One slot of the one-hot output. The k-step gate chain is sequential
  // (each step materializes the previous Cipher before the next gate call),
  // but slots are independent -- farm them across threads instead of
  // calling exec directly if needed.
  Cipher<Lwe, Rlwe> exec_slot_impl(uint32_t h,
                                   const std::vector<TLWE<Torus, n>>& v) const {
    TLWE<Torus, n> w;
    w.b() = Torus(1u, 4u);
    Cipher<Lwe, Rlwe> acc = w;

    for (size_t i = 0; i < k; ++i) {
      uint32_t bit = (h >> i) & 1u;
      Cipher<Lwe, Rlwe> vi = v[i];

      // No-op if acc is already Lwe-shaped (the FuseKeySwitch branch below).
      relay_.materialize(acc);

      if constexpr (FuseKeySwitch) {
        acc = bit ? Cipher<Lwe, Rlwe>(
                        tfhe::gate::HomAnd<Lwe, Rlwe, Decomp, Backend>::
                            template exec_impl<Kst>(acc.ready(), vi.ready(),
                                                    *bk_, relay_.ksk()))
                  : Cipher<Lwe, Rlwe>(
                        tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp, Backend>::
                            template exec_impl<Kst>(acc.ready(), vi.ready(),
                                                    *bk_, relay_.ksk()));
      } else {
        acc =
            bit ? Cipher<Lwe, Rlwe>(
                      tfhe::gate::HomAnd<Lwe, Rlwe, Decomp, Backend>::exec_impl(
                          acc.ready(), vi.ready(), *bk_))
                : Cipher<Lwe, Rlwe>(
                      tfhe::gate::HomAndNot<Lwe, Rlwe, Decomp,
                                            Backend>::exec_impl(acc.ready(),
                                                                vi.ready(),
                                                                *bk_));
      }
    }
    return acc;
  }

  std::array<Cipher<Lwe, Rlwe>, H> exec(
      const std::vector<TLWE<Torus, n>>& v) const {
    return exec_impl(v, std::make_index_sequence<H>{});
  }

  // Forwards to this instance's own Relay -- a no-op if bit is already
  // Lwe-shaped.
  const TLWE<Torus, n>& materialize(Cipher<Lwe, Rlwe>& bit) const {
    return relay_.materialize(bit);
  }

 private:
  template <size_t... Hs>
  std::array<Cipher<Lwe, Rlwe>, H> exec_impl(
      const std::vector<TLWE<Torus, n>>& v, std::index_sequence<Hs...>) const {
    return {exec_slot_impl(static_cast<uint32_t>(Hs), v)...};
  }

  const BootstrapKey<rTorus, N, l, n>* bk_;
  Relay<Lwe, Rlwe, Kst> relay_;
};

}  // namespace tfhe::circuit

#endif
