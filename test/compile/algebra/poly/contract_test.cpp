#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "algebra/poly.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

static_assert(std::ranges::contiguous_range<Poly<UInt>>);
static_assert(std::ranges::contiguous_range<Poly<ModTorus<16>>>);
static_assert(std::ranges::contiguous_range<Poly<ModInt<7>>>);
