#include "primitive/concept/scalar.hpp"
#include "primitive/torus.hpp"

// ============================================================
// Torus
// ============================================================

static_assert(TrivialScalar<detail::Torus>);
static_assert(
    CompactScalar<detail::Torus, typename detail::Torus::raw_value_type>);

static_assert(NothrowConstructible<detail::Torus,
                                   typename detail::Torus::raw_value_type>);

static_assert(NothrowExplicitlyConvertible<detail::Torus, ModTorus<16>>);
static_assert(NothrowExplicitlyConvertible<
              detail::Torus, typename detail::Torus::raw_value_type>);

// ============================================================
// ModTorus
// ============================================================

static_assert(TrivialScalar<ModTorus<16>>);
static_assert(
    CompactScalar<ModTorus<16>, typename ModTorus<16>::raw_value_type>);

static_assert(
    NothrowConstructible<ModTorus<16>, typename ModTorus<16>::raw_value_type>);

static_assert(NothrowExplicitlyConvertible<ModTorus<16>, detail::Torus>);
static_assert(NothrowExplicitlyConvertible<
              ModTorus<16>, typename ModTorus<16>::raw_value_type>);