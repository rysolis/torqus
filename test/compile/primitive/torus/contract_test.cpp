#include "primitive/concept/scalar.hpp"
#include "primitive/torus.hpp"

// ============================================================
// Torus
// ============================================================

static_assert(TrivialScalar<Torus>);
static_assert(CompactScalar<Torus, Torus::raw_value_type>);

static_assert(NothrowConstructible<Torus, Torus::raw_value_type>);

static_assert(NothrowExplicitlyConvertible<Torus, ModTorus>);
static_assert(NothrowExplicitlyConvertible<Torus, Torus::raw_value_type>);

// ============================================================
// ModTorus
// ============================================================

static_assert(TrivialScalar<ModTorus>);
static_assert(CompactScalar<ModTorus, ModTorus::raw_value_type>);

static_assert(NothrowConstructible<ModTorus, ModTorus::raw_value_type>);

static_assert(NothrowExplicitlyConvertible<ModTorus, Torus>);
static_assert(NothrowExplicitlyConvertible<ModTorus, ModTorus::raw_value_type>);