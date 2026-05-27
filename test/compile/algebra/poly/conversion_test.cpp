#include <concepts>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"

// clang-format off
static_assert(
  requires(Poly<ModTorus> p) {
    { convert_to<Torus>(p) } 
        -> std::same_as<Poly<Torus>>;
  }
);

static_assert(
  requires(Poly<ModTorus> p) {
    { convert_to<Torus>(std::move(p)) }
        -> std::same_as<Poly<Torus>>;
  }
);

static_assert(
  requires(Poly<Torus> p) {
    { convert_to<ModTorus>(p) }
        -> std::same_as<Poly<ModTorus>>;
  }
);

static_assert(
  requires(Poly<Torus> p) {
    { convert_to<ModTorus>(std::move(p)) }
        -> std::same_as<Poly<ModTorus>>;
  }
);
// clang-format on