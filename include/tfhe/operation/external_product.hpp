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

template <decompose_concept params>
class ExternalProduct {
 public:
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t B = params::B;
  static constexpr uint32_t l = params::l;

  template <torus_type Torus>
  inline static TRLWE<Torus, N> exec(const TRGSW<Torus, N, l>& trgsw,
                                     const TRLWE<Torus, N>& trlwe) {
    GadgetTRLWE<params> gd(trlwe);
    TRLWE<Torus, N> res;
    for (size_t i = 0; i < params::l; ++i) {
      res.a() += negacyclic_convolution(gd.a()[i], trgsw[i].a());
      res.a() += negacyclic_convolution(gd.b()[i], trgsw[l + i].a());
      res.b() += negacyclic_convolution(gd.a()[i], trgsw[i].b());
      res.b() += negacyclic_convolution(gd.b()[i], trgsw[l + i].b());
    }
    return res;
  }
};

#endif  // TFHE_EXTERNAL_PRODUCT_HPP