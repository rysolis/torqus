#include <concepts>
#include <type_traits>

// ============================================================
// trivial scalar representation
// ============================================================

template <typename T>
concept TrivialScalar =
    std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

// ============================================================
// compact scalar representation
// ============================================================

template <typename T, typename Raw>
concept CompactScalar = sizeof(T) == sizeof(Raw);

// ============================================================
// noexcept construction
// ============================================================

template <typename T, typename... Args>
concept NothrowConstructible =
    std::constructible_from<T, Args...> && noexcept(T(std::declval<Args>()...));

// ============================================================
// noexcept conversion
// ============================================================

template <typename From, typename To>
concept NothrowExplicitlyConvertible = requires(From x) {
  { static_cast<To>(x) } noexcept;
};
