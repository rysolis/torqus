#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

#include "algebra/poly.hpp"

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
