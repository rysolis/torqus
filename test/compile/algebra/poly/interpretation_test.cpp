#include <concepts>

#include "algebra/poly.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

// clang-format off
static_assert(
  requires(Poly<ModTorus<16>> p) {
    { interpret_as<UInt>(p) }
      -> std::same_as<Poly<UInt>>;
  }
);

static_assert(
  requires(Poly<ModTorus<16>> p) {
    { interpret_as<UInt>(std::move(p)) }
      -> std::same_as<Poly<UInt>>;
  }
);

static_assert(
  requires(Poly<ModInt<7>> p) {
    { interpret_as<UInt>(p) }
      -> std::same_as<Poly<UInt>>;
  }
);

static_assert(
  requires(Poly<ModInt<7>> p) {
    { interpret_as<UInt>(std::move(p)) }
      -> std::same_as<Poly<UInt>>;
  }
);

// clang-format on