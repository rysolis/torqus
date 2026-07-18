// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_EXTERNAL_PRODUCT_HPP
#define TFHE_EXTERNAL_PRODUCT_HPP

#include "primitive/concept/torus.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/structure/gadget/gadget_repr.hpp"

template <typename Rlwe, typename Dcp>
  requires trlwe_concept<Rlwe> && decompose_concept<Dcp>
class ExternalProduct {
 public:
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t B = Dcp::B;
  static constexpr uint32_t l = Dcp::l;

  template <torus_type Torus>
  inline static TRLWE<Torus, N> exec(const TRGSW<Torus, N, l>& trgsw,
                                     const TRLWE<Torus, N>& trlwe) {
    GadgetTRLWE<Rlwe, Dcp> gd(trlwe);
    TRLWE<Torus, N> res;
    for (size_t i = 0; i < Dcp::l; ++i) {
      res.a() += negacyclic_convolution(gd.a()[i], trgsw[i].a());
      res.a() += negacyclic_convolution(gd.b()[i], trgsw[l + i].a());
      res.b() += negacyclic_convolution(gd.a()[i], trgsw[i].b());
      res.b() += negacyclic_convolution(gd.b()[i], trgsw[l + i].b());
    }
    return res;
  }
};

#endif  // TFHE_EXTERNAL_PRODUCT_HPP