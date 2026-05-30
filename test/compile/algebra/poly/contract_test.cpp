#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "algebra/poly.hpp"
#include "primitive/modint.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"

// ============================================================
// Poly value semantics
// ============================================================

template <typename T>
concept PolyRegular =
    std::move_constructible<Poly<T>> && std::copy_constructible<Poly<T>> &&

    std::assignable_from<Poly<T>&, Poly<T>> &&
    std::assignable_from<Poly<T>&, const Poly<T>&> &&

    std::is_nothrow_move_constructible_v<Poly<T>> &&
    std::is_nothrow_move_assignable_v<Poly<T>> &&

    std::swappable<Poly<T>>;

// ============================================================
// Poly basic construction
// ============================================================

template <typename T>
concept PolySizeConstructible = std::constructible_from<Poly<T>, size_t>;

template <typename T>
concept PolyInitializerListConstructible =
    std::constructible_from<Poly<T>, std::initializer_list<T>>;

template <typename T, typename F>
concept PolyGeneratorConstructible =
    std::constructible_from<Poly<T>, size_t, F>;

// ============================================================
// Raw buffer construction
// ============================================================

template <typename T>
concept PolyRawPointerConstructible =
    std::constructible_from<Poly<T>, const typename T::raw_value_type*, size_t>;

template <typename T>
concept PolyUniquePtrConstructible = std::constructible_from<
    Poly<T>, std::unique_ptr<typename T::raw_value_type[]>, size_t>;

// ============================================================
// Torus
// ============================================================

static_assert(PolyRegular<detail::Torus>);
static_assert(PolySizeConstructible<detail::Torus>);
static_assert(PolyInitializerListConstructible<detail::Torus>);
static_assert(PolyGeneratorConstructible<detail::Torus, decltype([]() {
                                           return detail::Torus{};
                                         })>);
static_assert(PolyRawPointerConstructible<detail::Torus>);
static_assert(PolyUniquePtrConstructible<detail::Torus>);

// ============================================================
// ModTorus
// ============================================================

static_assert(PolyRegular<ModTorus<16>>);
static_assert(PolySizeConstructible<ModTorus<16>>);
static_assert(PolyInitializerListConstructible<ModTorus<16>>);
static_assert(PolyGeneratorConstructible<ModTorus<16>, decltype([]() {
                                           return ModTorus<16>{};
                                         })>);
static_assert(PolyRawPointerConstructible<ModTorus<16>>);
static_assert(PolyUniquePtrConstructible<ModTorus<16>>);

// ============================================================
// ModInt
// ============================================================

static_assert(PolyRegular<ModInt<7>>);
static_assert(PolySizeConstructible<ModInt<7>>);
static_assert(PolyInitializerListConstructible<ModInt<7>>);
static_assert(PolyGeneratorConstructible<ModInt<7>, decltype([]() {
                                           return ModInt<7>{};
                                         })>);
static_assert(PolyRawPointerConstructible<ModInt<7>>);
static_assert(PolyUniquePtrConstructible<ModInt<7>>);

// ============================================================
// UInt
// ============================================================

static_assert(PolyRegular<UInt>);
static_assert(PolySizeConstructible<UInt>);
static_assert(PolyInitializerListConstructible<UInt>);
static_assert(
    PolyGeneratorConstructible<UInt, decltype([]() { return UInt{}; })>);
static_assert(PolyRawPointerConstructible<UInt>);
static_assert(PolyUniquePtrConstructible<UInt>);