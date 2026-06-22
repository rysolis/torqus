#ifndef EXTERNAL_PRODUCT_HPP
#define EXTERNAL_PRODUCT_HPP

#include "primitive/concept/torus.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concepts.hpp"
#include "tfhe/structure/gadget_repr.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

template <decompose_params params>
class ExternalProduct {
 public:
  template <torus_type Torus>
  TRLWE<Torus, params::N> operator()(
      const TRGSW<Torus, params::N>& bk,
      const TRLWE<Torus, params::N>& trlwe) const {
    GadgetTRLWE<params> gd(trlwe);
    TRLWE<Torus, params::N> res;
    for (size_t i = 0; i < params::l; ++i) {
      res.a() += negacyclic_convolution(gd.a()[i], bk[i].a());
      res.a() += negacyclic_convolution(gd.b()[i], bk[params::l + i].a());
      res.b() += negacyclic_convolution(gd.a()[i], bk[i].b());
      res.b() += negacyclic_convolution(gd.b()[i], bk[params::l + i].b());
    }
    return res;
  }

  static constexpr double threshold =
      0.1;  // TODO: compute threshold by using params
};

#endif