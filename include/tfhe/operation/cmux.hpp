#ifndef CMUX_HPP
#define CMUX_HPP

#include "algebra/poly.hpp"
#include "primitive/concept/torus.hpp"
#include "tfhe/concepts.hpp"
#include "tfhe/operation/external_product.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

template <decompose_params params>
class CMux {
 public:
  template <torus_type Torus>
  TRLWE<Torus, params::N> operator()(
      const TRGSW<Torus, params::N>& bk, const TRLWE<Torus, params::N> cand0,
      const TRLWE<Torus, params::N> cand1) const {
    return this->extprod_(bk, (cand1 - cand0)) + cand0;
  }

  ExternalProduct<params> extprod_;
};

#endif