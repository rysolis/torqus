// Copyright 2026, EmotionX Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_GATE_BOOTSTRAP_HPP
#define TFHE_GATE_BOOTSTRAP_HPP

#include <cstdint>
#include <limits>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

#include "algebra/vector.hpp"

#include "tfhe/math/modswitch.hpp"
#include "tfhe/operation/bootstrap/blindrotate.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/operation/leveled/sample_extract.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

namespace {

// Overloads mod_switch (tfhe/math/modswitch.hpp) for a Torus argument:
// wraps it as a ModInt over its own modulus 2^QBit (0 standing in when
// that doesn't fit in Word -- see ModInt's "mod == 0" convention) and
// hands it to the ModInt-taking mod_switch, which is all that function
// knows about. Lives here rather than in modswitch.hpp itself so that
// file stays Torus-agnostic; ADL/argument-type overload resolution picks
// this one whenever the caller passes a ModTorus.
template <uint64_t M, uint32_t QBit, typename Word>
constexpr ModInt<M> mod_switch(const ModTorus<QBit, Word>& t) {
  constexpr uint64_t N = QBit == std::numeric_limits<Word>::digits
                             ? uint64_t{0}
                             : uint64_t{1} << QBit;
  return mod_switch<M>(ModInt<N, Word>(t.value()));
}

}  // namespace

namespace tfhe::bootstrap {

template <typename Lwe, typename Rlwe, typename Decomp>
class GateBootstrap {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;
  static constexpr uint32_t M = 2 * N;

  static constexpr uint32_t l = Decomp::l;

  static TLWE<rTorus, N> exec_impl(const rTorus mu, const TRLWE<rTorus, N>& tv,
                                   const TLWE<Torus, n>& tlwe,
                                   const BootstrapKey<rTorus, N, l, n>& bk) {
    // convert TLWE to Vector<ModInt<M>>
    Vector<ModInt<M>, n + 1> amount;
    for (size_t i = 0; i < n; ++i) {
      amount[i] = mod_switch<M>(tlwe.a()[i]);
    }
    amount[n] = mod_switch<M>(tlwe.b());

    // BlindRotate
    TRLWE<rTorus, N> rot =
        BlindRotate<Lwe, Rlwe, Decomp>::exec_impl(tv, amount, bk);

    // prepare offset
    TLWE<rTorus, N> offset;
    offset.b() = rTorus(mu.value() / 2);

    return leveled::Add<Rlwe>::exec_impl(
        offset, leveled::SampleExtract<Lwe, Rlwe>::exec_impl(rot, 0));
  }
};

}  // namespace tfhe::bootstrap

#endif
