#include "primitive/concept/convertible.hpp"
#include "primitive/concept/primitive.hpp"
#include "primitive/modint.hpp"

// ModInt<P> ======================
static_assert(primitive<ModInt<7>>);
static_assert(primitive<ModInt<(1u << 31) - 1>>);

static_assert(!std::convertible_to<ModInt<7>::raw_value_type, ModInt<7>>);
static_assert(explicitly_convertible_to<ModInt<7>::raw_value_type, ModInt<7>>);
static_assert(!std::convertible_to<ModInt<(1u << 31) - 1>::raw_value_type,
                                   ModInt<(1u << 31) - 1>>);
static_assert(explicitly_convertible_to<ModInt<(1u << 31) - 1>::raw_value_type,
                                        ModInt<(1u << 31) - 1>>);
// ==================================