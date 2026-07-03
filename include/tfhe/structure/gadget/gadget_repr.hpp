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

template <decompose_concept params, torus_type Torus>
UInt decompose(const Torus& v, size_t i) {
  static constexpr uint32_t Bbit = std::bit_width(params::B - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  UInt::raw_value_type w =
      UInt::raw_value_type(static_cast<Torus::raw_value_type>(v));
  UInt::raw_value_type tmp = (w >> shift) & (params::B - 1);
  return UInt(tmp);
}

template <decompose_concept params, torus_type Torus>
Torus reconstruct(const Vector<Poly<UInt, params::N>, params::l>& repr,
                  size_t j) {
  static constexpr uint32_t Bbit = std::bit_width(params::B - 1);
  typename Torus::raw_value_type m = 0;
  for (size_t i = 0; i < params::l; ++i) {
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
template <decompose_concept params, torus_type Torus>
UInt decompose(const Torus& v, size_t i) {
  static constexpr uint32_t Bbit = std::bit_width(params::B - 1);
  size_t shift = Torus::qbit - (Bbit * (i + 1));
  assert(shift <= (Torus::qbit - Bbit));

  // To mitigate the effect of quantization error, we add a rounding offset
  // before extraction.
  UInt::raw_value_type round = 0;
  if constexpr (Torus::qbit - (Bbit * params::l) > 0) {
    round = 1u << (Torus::qbit - (Bbit * params::l) - 1);
  }
  UInt::raw_value_type offset = 0;
  for (size_t i = 0; i < params::l; ++i) {
    offset += (params::B / 2) << (Torus::qbit - (Bbit * (i + 1)));
  }
  UInt::raw_value_type w = UInt::raw_value_type(
      static_cast<Torus::raw_value_type>(v) + offset + round);
  UInt::raw_value_type tmp = ((w >> shift) & (params::B - 1)) - (params::B / 2);
  return UInt(tmp);
}

template <decompose_concept params, torus_type Torus>
Torus reconstruct(const Vector<Poly<UInt, params::N>, params::l>& repr,
                  size_t j) {
  static constexpr uint32_t Bbit = std::bit_width(params::B - 1);
  UInt::raw_value_type offset = 0;
  for (size_t i = 0; i < params::l; ++i) {
    offset += (params::B / 2) << (Torus::qbit - (Bbit * (i + 1)));
  }
  typename Torus::raw_value_type m = 0;
  for (size_t i = 0; i < params::l; ++i) {
    size_t shift = Torus::qbit - (Bbit * (i + 1));
    assert(shift <= (Torus::qbit - Bbit));

    UInt::raw_value_type v = static_cast<UInt::raw_value_type>(repr[i][j]);
    m |= ((v + (params::B / 2)) & (params::B - 1)) << shift;
  }
  m -= offset;
  return Torus(m);
}

}  // namespace balanced

}  // namespace

template <decompose_concept params>
class GadgetRepr {
 public:
  static constexpr uint32_t N = params::N;
  static constexpr uint32_t l = params::l;
  static constexpr uint32_t B = params::B;
  static constexpr uint32_t Bbit = std::bit_width(B - 1);

  // Torus shoule be ModTorus<QBit> !!
  template <torus_type Torus>
  explicit GadgetRepr(const Poly<Torus, N>& poly) {
    static_assert(Torus::qbit >= Bbit * l,
                  "Torus qbit must be greater than or equal to Bbit * l");
    for (size_t j = 0; j < N; ++j) {
      for (size_t i = 0; i < l; ++i) {
        repr_[i][j] = balanced::decompose<params>(poly[j], i);
      }
    }
  }

  template <typename Torus>
  inline Poly<Torus, N> reconstruct() const {
    Poly<Torus, N> poly;

    for (size_t j = 0; j < N; ++j) {
      poly[j] = balanced::reconstruct<params, Torus>(repr_, j);
    }

    return poly;
  }

  Poly<UInt, N>& operator[](size_t idx) noexcept { return repr_[idx]; }
  Poly<UInt, N> operator[](size_t idx) const noexcept { return repr_[idx]; }

  friend std::ostream& operator<<(std::ostream& os, const GadgetRepr& repr) {
    for (size_t i = 0; i < repr.repr_.size(); ++i) {
      os << "repr[" << i << "]: " << repr.repr_[i] << "\n";
    }
    return os;
  }

  // 2^(qbit-Bbit*l) / 2^qbit
  // = 2^(-(Bbit*l))
  template <torus_type Torus>
  static constexpr double threshold = 1.0 / (1ULL << (Bbit * l));

 private:
  Vector<Poly<UInt, N>, l> repr_;
};

template <typename params>
class GadgetTRLWE {
 public:
  // NOLINT(bugprone-easily-swappable-parameters)
  GadgetTRLWE(GadgetRepr<params> a, GadgetRepr<params> b)
      : a_(std::move(a)), b_(std::move(b)) {}

  template <typename Torus>
  explicit GadgetTRLWE(const TRLWE<Torus, params::N>& trlwe)
      : a_(trlwe.a()), b_(trlwe.b()) {}

  [[nodiscard]]
  const GadgetRepr<params>& a() const noexcept {
    return a_;
  }

  [[nodiscard]]
  const GadgetRepr<params>& b() const noexcept {
    return b_;
  }

 private:
  GadgetRepr<params> a_;
  GadgetRepr<params> b_;
};

#endif  // TFHE_GADGET_REPR_HPP