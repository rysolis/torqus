#ifndef NEGACYCLIC_CONVOLUTION_HPP
#define NEGACYCLIC_CONVOLUTION_HPP

#include "primitive/concept/torus.hpp"

#include "algebra/poly.hpp"

#include "arithmetic/expr_impl.hpp"
#include "arithmetic/expr_trait.hpp"

template <typename Lhs, typename Rhs>
class NegacyclicConvolutionExpr {
 public:
  NegacyclicConvolutionExpr(const Lhs& lhs, const Rhs& rhs)
      : lhs_(lhs), rhs_(rhs) {}

  const Lhs& lhs_;
  const Rhs& rhs_;
};

template <typename Lhs, typename Rhs>
struct evaluated_type<NegacyclicConvolutionExpr<Lhs, Rhs>> {
  using type = Poly<typename Rhs::value_type, Rhs::size()>;
};

template <typename Lhs, typename Rhs>
NegacyclicConvolutionExpr<Lhs, Rhs> negacyclic_convolution(const Lhs& lhs,
                                                           const Rhs& rhs) {
  return NegacyclicConvolutionExpr<Lhs, Rhs>(lhs, rhs);
}

template <typename Lhs, typename Rhs>
struct accumulate_impl<NegacyclicConvolutionExpr<Lhs, Rhs>> {
  template <typename Out, typename AddOp, typename SubOp>
  static void apply(Out& dist, const NegacyclicConvolutionExpr<Lhs, Rhs>& ep,
                    AddOp add_op, SubOp sub_op) {
    using lhs_type = evaluated_type_t<Lhs>;
    using rhs_type = evaluated_type_t<Rhs>;

    lhs_type lhs(ep.lhs_);
    rhs_type rhs(ep.rhs_);

    constexpr size_t N = rhs_type::size();

    for (size_t i = 0; i < N; ++i) {
      for (size_t j = 0; j < N; ++j) {
        const auto term = static_cast<lhs_type::value_type>(lhs[i]) *
                          static_cast<rhs_type::value_type>(rhs[j]);
        const size_t k = i + j;

        if (k < N) {
          add_op(dist[k], term);
        } else {
          sub_op(dist[k - N], term);
        }
      }
    }
  }
};

#endif