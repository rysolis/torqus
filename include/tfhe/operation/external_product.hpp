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
  TRLWE<Torus, N> operator()(const TRGSW<Torus, N, l>& bk,
                             const TRLWE<Torus, N>& trlwe) const {
    double error_bound = compute_error_bound(bk, trlwe);
#ifndef NDEBUG
    if (error_bound >= 0.25) {
      std::cerr << "error_bound = " << error_bound << '\n';
    }
    assert(error_bound < 0.25);
#endif

    GadgetTRLWE<params> gd(trlwe);
    TRLWE<Torus, N> res;
    for (size_t i = 0; i < params::l; ++i) {
      res.a() += negacyclic_convolution(gd.a()[i], bk[i].a());
      res.a() += negacyclic_convolution(gd.b()[i], bk[l + i].a());
      res.b() += negacyclic_convolution(gd.a()[i], bk[i].b());
      res.b() += negacyclic_convolution(gd.b()[i], bk[l + i].b());
    }
    res.update_bound(error_bound);
    return res;
  }

  static constexpr double ep = 1. / (B << (l + 1));

  template <torus_type Torus>
  double compute_error_bound(const TRGSW<Torus, N, l>& bk,
                             const TRLWE<Torus, N>& trlwe) const {
    return (2 * l * N * (B << 1) * bk.error_bound()) +
           ((1. / B) * (1 + N) * ep) + ((1. / B) * trlwe.error_bound());
  }
};

#endif  // TFHE_EXTERNAL_PRODUCT_HPP