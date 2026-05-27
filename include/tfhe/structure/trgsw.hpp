#ifndef TRGSW_HPP
#define TRGSW_HPP

#include <iostream>
#include <vector>

#include "primitive/concept/castable.hpp"
#include "primitive/torus.hpp"
#include "tfhe/structure/gadget_repr.hpp"
#include "tfhe/structure/trgsw.hpp"
#include "tfhe/structure/trlwe.hpp"

template <typename T = Torus>
class TRGSW {
 public:
  TRGSW() = default;
  TRGSW(uint32_t N, uint32_t l) : trgsw_(2 * l, TRLWE<T>(N)), l_(l) {}

  TRLWE<T>& operator[](size_t idx) noexcept { return trgsw_[idx]; }
  const TRLWE<T>& operator[](size_t idx) const noexcept { return trgsw_[idx]; }

  uint32_t level() const noexcept { return 2 * l_; }

  friend std::ostream& operator<<(std::ostream& os, const TRGSW& trgsw) {
    os << "TRGSW\n";
    for (size_t i = 0; i < trgsw.level(); ++i) {
      os << trgsw.trgsw_[i] << "\n";
    }
    os << "\n";
    return os;
  }

 private:
  std::vector<TRLWE<T>> trgsw_;
  uint32_t l_;
};

template <typename To, typename From>
  requires castable<To, From>
inline TRGSW<To> convert_to(const TRGSW<From>& src) {
  TRGSW<To> dst(static_cast<uint32_t>(src[0].a().size()), src.level() >> 1);
  for (size_t i = 0; i < src.level(); ++i) {
    dst[i] = convert_to<To>(src[i]);
  }
  return dst;
}

#endif