#include "primitive/concept/scalar.hpp"
#include "primitive/modint.hpp"

// ============================================================
// ModInt
// ============================================================

static_assert(TrivialScalar<ModInt<7>>);
static_assert(TrivialScalar<ModInt<(1u << 31) - 1>>);

static_assert(NothrowConstructible<ModInt<7>, ModInt<7>::raw_value_type>);
static_assert(NothrowConstructible<ModInt<(1u << 31) - 1>,
                                   ModInt<(1u << 31) - 1>::raw_value_type>);