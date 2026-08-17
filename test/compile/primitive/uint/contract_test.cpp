#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"
#include "primitive/uint.hpp"

// UInt ============================
static_assert(primitive_concept<UInt>);

static_assert(!std::convertible_to<UInt::raw_value_type, UInt>);
static_assert(explicitly_convertible_to_concept<UInt::raw_value_type, UInt>);
// ==================================
