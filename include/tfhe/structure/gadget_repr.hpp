#ifndef GADGET_REPR_HPP
#define GADGET_REPR_HPP

#include <bit>
#include <concepts>
#include <vector>

#include "algebra/poly.hpp"
#include "primitive/concept/torus.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"
#include "tfhe/adapter/adapter.hpp"
#include "tfhe/structure/trlwe.hpp"

template <typename Ctx, typename params = decompose::params<Ctx>>
  requires std::derived_from<params, decompose::tag>
class GadgetRepr {
 public:
  // Torus shoule be ModTorus<QBit> !!
  template <TorusType Torus>
  explicit GadgetRepr(const Poly<Torus>& poly)
      : repr_(params::l, Poly<UInt>(params::N)) {
    static_assert(Torus::qbit >= Bbit_ * params::l,
                  "Torus qbit must be greater than or equal to Bbit * l");

    auto extract = [Bbit = Bbit_](const UInt::raw_value_type m,
                                  const size_t idx) -> UInt {
      size_t shift = Torus::qbit - (Bbit * (idx + 1));
      assert(shift <= (Torus::qbit - Bbit));
      UInt::raw_value_type tmp = (m >> shift) & (params::B - 1);
      assert(tmp < params::B);
      return static_cast<UInt>(tmp);
    };

    for (size_t j = 0; j < params::N; ++j) {
      for (size_t i = 0; i < params::l; ++i) {
        Torus v = poly[j];
        UInt w = UInt(static_cast<UInt::raw_value_type>(
            static_cast<Torus::raw_value_type>(v)));
        repr_[i][j] = extract(static_cast<UInt::raw_value_type>(w), i);
      }
    }
  }

  template <typename Torus>
  inline Poly<Torus> reconstruct() const {
    Poly<Torus> poly(params::N);

    for (size_t j = 0; j < params::N; ++j) {
      UInt::raw_value_type m = 0;
      for (size_t i = 0; i < params::l; ++i) {
        size_t shift = Torus::qbit - (Bbit_ * (i + 1));
        assert(shift <= (Torus::qbit - Bbit_));
        UInt::raw_value_type v = static_cast<UInt::raw_value_type>(repr_[i][j]);
        m |= v << shift;
      }
      poly[j] = Torus(m);
    }

    return poly;
  }

  Poly<UInt>& operator[](size_t idx) noexcept { return repr_[idx]; }
  Poly<UInt> operator[](size_t idx) const noexcept { return repr_[idx]; }

  friend std::ostream& operator<<(std::ostream& os, const GadgetRepr& repr) {
    for (size_t i = 0; i < repr.repr_.size(); ++i) {
      os << "repr[" << i << "]: " << repr.repr_[i] << "\n";
    }
    return os;
  }

  static constexpr double threshold =
      1.0 / (1ULL << (std::bit_width(params::B - 1) * params::l));

 private:
  std::vector<Poly<UInt>> repr_;
  static constexpr size_t Bbit_{std::bit_width(params::B - 1)};
};

template <typename Ctx>
class GadgetTRLWE {
 public:
  // NOLINT(bugprone-easily-swappable-parameters)
  GadgetTRLWE(GadgetRepr<Ctx> a, GadgetRepr<Ctx> b)
      : a_(std::move(a)), b_(std::move(b)) {}

  template <typename Torus>
  explicit GadgetTRLWE(const TRLWE<Torus>& trlwe)
      : a_(trlwe.a()), b_(trlwe.b()) {}

  [[nodiscard]]
  const GadgetRepr<Ctx>& a() const noexcept {
    return a_;
  }

  [[nodiscard]]
  const GadgetRepr<Ctx>& b() const noexcept {
    return b_;
  }

 private:
  GadgetRepr<Ctx> a_;
  GadgetRepr<Ctx> b_;
};

#endif