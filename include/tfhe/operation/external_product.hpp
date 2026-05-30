#ifndef EXTERNAL_PRODUCT_HPP
#define EXTERNAL_PRODUCT_HPP

#include "arithmetic/multiplication.hpp"
#include "tfhe/adapter/adapter.hpp"
#include "tfhe/structure/gadget_repr.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

template <typename Ctx>
class ExternalProduct {
 public:
  template <TorusType Torus>
  TRLWE<Torus> operator()(const TRGSW<Torus>& bk,
                          const TRLWE<Torus>& trlwe) const {
    GadgetTRLWE<Ctx> gd(trlwe);
    TRLWE<Torus> res(Ctx::N);
    for (size_t i = 0; i < Ctx::l; ++i) {
      res.a() += gd.a()[i] * bk[i].a();
      res.a() += gd.b()[i] * bk[Ctx::l + i].a();
      res.b() += gd.a()[i] * bk[i].b();
      res.b() += gd.b()[i] * bk[Ctx::l + i].b();
    }
    return res;
  }

  static constexpr double threshold =
      0.1;  // TODO: compute threshold by using Ctx
};

#endif