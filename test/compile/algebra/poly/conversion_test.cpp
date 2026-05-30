#include <concepts>

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"

// clang-format off
static_assert(
  requires(Poly<ModTorus<16>> p) {
    { convert_to<detail::Torus>(p) } 
        -> std::same_as<Poly<detail::Torus>>;
  }
);

static_assert(
  requires(Poly<ModTorus<16>> p) {
    { convert_to<detail::Torus>(std::move(p)) }
        -> std::same_as<Poly<detail::Torus>>;
  }
);

static_assert(
  requires(Poly<detail::Torus> p) {
    { convert_to<ModTorus<16>>(p) }
        -> std::same_as<Poly<ModTorus<16>>>;
  }
);

static_assert(
  requires(Poly<detail::Torus> p) {
    { convert_to<ModTorus<16>>(std::move(p)) }
        -> std::same_as<Poly<ModTorus<16>>>;
  }
);
// clang-format on