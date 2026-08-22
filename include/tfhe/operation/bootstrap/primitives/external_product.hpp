// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_EXTERNAL_PRODUCT_HPP
#define TFHE_EXTERNAL_PRODUCT_HPP

#include "primitive/concept/torus.hpp"

#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/gadget/gadget_repr.hpp"

namespace tfhe::bootstrap {

template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
class ExternalProduct {
 public:
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t l = Decomp::l;

  template <torus_concept Torus>
  static TRLWE<Torus, N> exec_impl(const TRGSW<Torus, N, l>& trgsw,
                                   const TRLWE<Torus, N>& trlwe) {
    GadgetTRLWE<Rlwe, Decomp> gadget(trlwe);
    TRLWE<Torus, N> res;
    for (size_t i = 0; i < Decomp::l; ++i) {
      res.a() += negacyclic_convolution(gadget.a()[i], trgsw[i].a());
      res.a() += negacyclic_convolution(gadget.b()[i], trgsw[l + i].a());
      res.b() += negacyclic_convolution(gadget.a()[i], trgsw[i].b());
      res.b() += negacyclic_convolution(gadget.b()[i], trgsw[l + i].b());
    }
    return res;
  }
};

}  // namespace tfhe::bootstrap

#endif  // TFHE_EXTERNAL_PRODUCT_HPP