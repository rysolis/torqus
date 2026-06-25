#ifndef EXTERNAL_PRODUCT_HPP
#define EXTERNAL_PRODUCT_HPP

#include "primitive/concept/torus.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/negacyclic_convolution.hpp"

#include "tfhe/concepts.hpp"
#include "tfhe/structure/gadget_repr.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

template <decompose_concept params>
class ExternalProduct {
 public:
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t B = params::B;
  static constexpr uint32_t l = params::l;

  template <torus_type Torus>
  TRLWE<Torus, N> operator()(const TRGSW<Torus, N>& bk,
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
  double compute_error_bound(const TRGSW<Torus, N>& bk,
                             const TRLWE<Torus, N>& trlwe) const {
    return (2 * l * N * (B << 1) * bk.error_bound()) +
           ((1. / B) * (1 + N) * ep) + ((1. / B) * trlwe.error_bound());
  }
};

#endif