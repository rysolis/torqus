#ifndef VECTOR_HPP
#define VECTOR_HPP

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>

#include "primitive/concept/convertible.hpp"

#include "detail/proxy.hpp"

template <typename T, uint32_t n>
class Vector {
 public:
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;

  using iterator = raw_value_type*;
  using const_iterator = const raw_value_type*;

  Vector() = default;

  Vector(const Vector& other) {
    std::copy_n(other.data_.get(), n, data_.get());
  }

  Vector& operator=(const Vector& other) {
    if (this == &other) return *this;
    std::copy_n(other.data_.get(), n, data_.get());
    return *this;
  }

  Vector(Vector&&) noexcept = default;
  Vector& operator=(Vector&&) noexcept = default;

  template <typename F>
    requires requires(F& f, std::size_t i) {
      { std::invoke(f, i) } -> explicitly_convertible_to<T>;
    }
  Vector(F&& f) {
    std::size_t i = 0;
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f, i++));
    });
  }

  template <typename F>
    requires(
        !requires(F& f, std::size_t i) {
          { std::invoke(f, i) } -> explicitly_convertible_to<T>;
        } &&
        requires(F& f) {
          { std::invoke(f) } -> explicitly_convertible_to<T>;
        })
  Vector(F&& f) {
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f));
    });
  }

  Proxy<Vector> operator[](size_t idx) noexcept {
    return Proxy<Vector>(data_.get() + idx);
  }
  value_type operator[](size_t idx) const noexcept {
    return static_cast<value_type>(data_[idx]);
  }

  constexpr iterator begin() noexcept { return data_.get(); }
  constexpr iterator end() noexcept { return data_.get() + n; }

  constexpr const_iterator begin() const noexcept { return data_.get(); }
  constexpr const_iterator end() const noexcept { return data_.get() + n; }

  T* data() { return data_.get(); }
  const T* data() const { return data_.get(); }

  constexpr size_t size() const { return n; }

  friend std::ostream& operator<<(std::ostream& os, const Vector& vec) {
    os << "Vec(";
    for (size_t i = 0; i < vec.size(); ++i) {
      os << vec[i];
      if (i + 1 < vec.size()) {
        os << ", ";
      }
    }
    os << ")";
    return os;
  }

 private:
  std::unique_ptr<raw_value_type[]> data_ =
      std::make_unique<raw_value_type[]>(n);
};

#endif