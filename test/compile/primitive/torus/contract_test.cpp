#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"
#include "primitive/torus.hpp"

// detail::Torus ====================
static_assert(!primitive<detail::Torus>);

static_assert(
    !std::convertible_to<detail::Torus::raw_value_type, detail::Torus>);
static_assert(
    explicitly_convertible_to<detail::Torus::raw_value_type, detail::Torus>);

static_assert(explicitly_convertible_to<detail::Torus, ModTorus<16>>);

static_assert(
    !std::convertible_to<detail::Torus::raw_value_type, ModTorus<16>>);
static_assert(
    !explicitly_convertible_to<detail::Torus::raw_value_type, ModTorus<16>>);
// ==================================

// ModTorus<Q> ======================
static_assert(primitive<ModTorus<16>>);

static_assert(!std::convertible_to<ModTorus<16>::raw_value_type, ModTorus<16>>);
static_assert(
    explicitly_convertible_to<ModTorus<16>::raw_value_type, ModTorus<16>>);

static_assert(explicitly_convertible_to<ModTorus<16>, detail::Torus>);
// ==================================
