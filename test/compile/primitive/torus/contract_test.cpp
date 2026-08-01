#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"
#include "primitive/torus.hpp"

// dbl::Torus ====================
static_assert(!primitive<dbl::Torus>);

static_assert(!std::convertible_to<dbl::Torus::raw_value_type, dbl::Torus>);
static_assert(
    explicitly_convertible_to<dbl::Torus::raw_value_type, dbl::Torus>);

static_assert(explicitly_convertible_to<dbl::Torus, ModTorus<16>>);

static_assert(!std::convertible_to<dbl::Torus::raw_value_type, ModTorus<16>>);
static_assert(
    !explicitly_convertible_to<dbl::Torus::raw_value_type, ModTorus<16>>);
// ==================================

// ModTorus<Q> ======================
static_assert(primitive<ModTorus<16>>);

static_assert(!std::convertible_to<ModTorus<16>::raw_value_type, ModTorus<16>>);
static_assert(
    explicitly_convertible_to<ModTorus<16>::raw_value_type, ModTorus<16>>);

static_assert(explicitly_convertible_to<ModTorus<16>, dbl::Torus>);
// ==================================
