// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_GADGET_REPR_HPP
#define TFHE_GADGET_REPR_HPP

#include <bit>
#include <bitset>
#include <concepts>
#include <cstdint>
#include <iostream>

#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"

namespace {

namespace classical {

template <decompose_concept Params, torus_concept Torus>
UInt decompose(const Torus& v, size_t i) {
  static constexpr uint32_t Bbit = std::bit_width(Params::B - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  UInt::raw_value_type w =
      UInt::raw_value_type(static_cast<Torus::raw_value_type>(v));
  UInt::raw_value_type tmp = (w >> shift) & (Params::B - 1);
  return UInt(tmp);
}

template <typename Rlwe, typename Decomp, torus_concept Torus>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
Torus reconstruct(const Vector<Poly<UInt, Rlwe::N>, Decomp::l>& repr,
                  size_t j) {
  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  typename Torus::raw_value_type m = 0;
  for (size_t i = 0; i < l; ++i) {
    size_t shift = Torus::qbit - (Bbit * (i + 1));
    assert(shift <= (Torus::qbit - Bbit));
    UInt::raw_value_type v = static_cast<UInt::raw_value_type>(repr[i][j]);
    m |= v << shift;
  }
  return Torus(m);
}

}  // namespace classical

namespace balanced {

// Compute {d_i} s.t. v = sum_i (d_i * B^i), where d_i in [-B/2, B/2).
//  v = sum_i (d_i * B^i) = sum_i (e_i - (Bg/2)) * B^i = sum_i (e_i * B^i) -
// (Bg/2) * sum_i (B^i).
// Therfore, we add an offset of (Bg/2) * sum_i (B^i) to v before decomposition.
template <decompose_concept Params, torus_concept Torus>
UInt decompose(const Torus& v, size_t i) {
  static constexpr uint32_t Bbit = std::bit_width(Params::B - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  // To mitigate the effect of quantization error, we add a rounding offset
  // before extraction.
  UInt::raw_value_type round = 0;
  if constexpr (Torus::qbit - (Bbit * Params::l) > 0) {
    round = 1u << (Torus::qbit - (Bbit * Params::l) - 1);
  }
  UInt::raw_value_type offset = 0;
  for (size_t i = 0; i < Params::l; ++i) {
    offset += (Params::B / 2) << (Torus::qbit - (Bbit * (i + 1)));
  }
  UInt::raw_value_type w = UInt::raw_value_type(
      static_cast<Torus::raw_value_type>(v) + offset + round);
  UInt::raw_value_type tmp = ((w >> shift) & (Params::B - 1)) - (Params::B / 2);
  return UInt(tmp);
}

template <typename Rlwe, typename Decomp, torus_concept Torus>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
Torus reconstruct(const Vector<Poly<UInt, Rlwe::N>, Decomp::l>& repr,
                  size_t j) {
  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  UInt::raw_value_type offset = 0;
  for (size_t i = 0; i < l; ++i) {
    offset += (B / 2) << (Torus::qbit - (Bbit * (i + 1)));
  }
  typename Torus::raw_value_type m = 0;
  for (size_t i = 0; i < l; ++i) {
    size_t shift = Torus::qbit - (Bbit * (i + 1));
    assert(shift <= (Torus::qbit - Bbit));

    UInt::raw_value_type v = static_cast<UInt::raw_value_type>(repr[i][j]);
    m |= ((v + (B / 2)) & (B - 1)) << shift;
  }
  m -= offset;
  return Torus(m);
}

}  // namespace balanced

}  // namespace

template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
class GadgetRepr {
 public:
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;
  static constexpr uint32_t B = Decomp::B;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  // Torus shoule be ModTorus<QBit> !!
  template <torus_concept Torus>
  explicit GadgetRepr(const Poly<Torus, N>& poly) {
    static_assert(Torus::qbit >= Bbit * l,
                  "Torus qbit must be greater than or equal to Bbit * l");
    for (size_t j = 0; j < N; ++j) {
      for (size_t i = 0; i < l; ++i) {
        repr_[i][j] =
            balanced::decompose<Decomp>(static_cast<Torus>(poly[j]), i);
      }
    }
  }

  template <typename Torus>
  inline Poly<Torus, N> reconstruct() const {
    Poly<Torus, N> poly;

    for (size_t j = 0; j < N; ++j) {
      poly[j] = balanced::reconstruct<Rlwe, Decomp, Torus>(repr_, j);
    }

    return poly;
  }

  Poly<UInt, N>& operator[](size_t idx) noexcept { return repr_[idx]; }
  const Poly<UInt, N>& operator[](size_t idx) const noexcept {
    return repr_[idx];
  }

  friend std::ostream& operator<<(std::ostream& os, const GadgetRepr& repr) {
    for (size_t i = 0; i < repr.repr_.size(); ++i) {
      os << "repr[" << i << "]: " << repr.repr_[i] << "\n";
    }
    return os;
  }

  // 2^(qbit-Bbit*l) / 2^qbit
  // = 2^(-(Bbit*l))
  template <torus_concept Torus>
  static constexpr double threshold = 1.0 / (1ULL << (Bbit * l));

 private:
  Vector<Poly<UInt, N>, l> repr_;
};

template <typename Rlwe, typename Decomp>
  requires trlwe_concept<Rlwe> && decompose_concept<Decomp>
class GadgetTRLWE {
 public:
  // NOLINT(bugprone-easily-swappable-parameters)
  GadgetTRLWE(GadgetRepr<Rlwe, Decomp> a, GadgetRepr<Rlwe, Decomp> b)
      : a_(std::move(a)), b_(std::move(b)) {}

  template <typename Torus>
  explicit GadgetTRLWE(const TRLWE<Torus, Rlwe::N>& trlwe)
      : a_(trlwe.a()), b_(trlwe.b()) {}

  [[nodiscard]]
  const GadgetRepr<Rlwe, Decomp>& a() const noexcept {
    return a_;
  }

  [[nodiscard]]
  const GadgetRepr<Rlwe, Decomp>& b() const noexcept {
    return b_;
  }

 private:
  GadgetRepr<Rlwe, Decomp> a_;
  GadgetRepr<Rlwe, Decomp> b_;
};

#endif  // TFHE_GADGET_REPR_HPP