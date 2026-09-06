// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_RESLOT_HPP
#define TFHE_RESLOT_HPP

#include <cstdint>

#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/utility/testvector.hpp"

// Bootstraps a single Lwe-shaped ciphertext to fresh noise while moving its
// encoded value from a 1/InResolution step to a 1/OutResolution step --
// e.g. a Bit lifted at Dial<2, Torus> (0 or 1/2) that needs to become
// Dial<4, Torus> (0 or 1/4) before feeding into And/Or/AndNot/Xor, which
// all fix their step at 1/4.
//
// GateBootstrap's test vector (testvector::generate) always splits at
// exactly 1/4 and 3/4 of the circle, classifying a message as the
// negative half centered on 0 or the positive half centered on 1/2 -- it
// has no separate threshold parameter. So rather than just offsetting the
// input (which only shifts where 0/mu_in land, without changing how far
// apart they are), this first scales the input up by InResolution/2 --
// via repeated self-addition, since no scalar-multiply leveled op exists
// -- so its two possible values become exactly {0, 1/2}: dead center of
// each half, for maximum noise margin. No offset is then needed at all.
// This is deliberately separate from Circuit::Refresh (scope.hpp), whose
// And(bit, bit) tautology is tied to HomAnd's own specific 3-input-sum ->
// 2-output classification and doesn't generalize to an arbitrary
// InResolution/OutResolution pair the way this does.
namespace tfhe::bootstrap {

template <typename Lwe, typename Rlwe, typename Decomp, uint32_t InResolution,
          uint32_t OutResolution>
class Reslot {
 public:
  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  static constexpr uint32_t l = Decomp::l;

  static_assert(InResolution % 2 == 0,
                "Reslot needs InResolution/2 doublings of the input to "
                "land its true value exactly on 1/2; an odd InResolution "
                "can't reach that by repeated self-addition");
  static constexpr uint32_t scale = InResolution / 2;

  static TLWE<rTorus, N> exec_impl(const TLWE<Torus, n>& c,
                                   const BootstrapKey<rTorus, N, l, n>& bk) {
    static constexpr rTorus mu_out(1u, OutResolution);

    TRLWE<rTorus, N> tv;
    tv.b() = testvector::generate<rTorus, N>(rTorus(mu_out.value() >> 1u));

    TLWE<Torus, n> scaled = c;
    for (uint32_t i = 1; i < scale; ++i) {
      scaled = leveled::Add<Lwe>::exec_impl(scaled, c);
    }

    return GateBootstrap<Lwe, Rlwe, Decomp>::exec_impl(mu_out, tv, scaled, bk);
  }
};

}  // namespace tfhe::bootstrap

#endif  // TFHE_RESLOT_HPP
