#include "algebra/poly.hpp"
#include "primitive/torus.hpp"

template <TorusType Torus>
inline constexpr double infinity_norm(const Poly<Torus>& poly) {
  double max_norm = 0.0;
  for (size_t i = 0; i < poly.size(); ++i) {
    double abs_val = infinity_norm(static_cast<Torus>(poly[i]));
    if (abs_val > max_norm) {
      max_norm = abs_val;
    }
  }
  return max_norm;
}
