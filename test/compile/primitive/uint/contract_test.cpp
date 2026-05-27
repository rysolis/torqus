#include "primitive/concept/scalar.hpp"
#include "primitive/uint.hpp"

// ============================================================
// UInt
// ============================================================

static_assert(TrivialScalar<UInt>);
static_assert(CompactScalar<UInt, UInt::raw_value_type>);

static_assert(NothrowConstructible<UInt, UInt::raw_value_type>);
