// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_CONTAINER_HPP
#define ALGEBRA_CONTAINER_HPP

#include <cstdint>
#include <initializer_list>
#include <memory>

#include "primitive/concept/primitive.hpp"

#include "detail/proxy.hpp"

template <typename T, typename = void>
struct storage_traits {
  using value_type = T;
  using raw_value_type = T;
  static constexpr bool use_proxy = false;
};

template <primitive_concept T>
struct storage_traits<T, std::void_t<typename T::raw_value_type>> {
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;
  static constexpr bool use_proxy = !std::same_as<T, raw_value_type>;
};

template <class Derived, typename T, uint32_t N,
          bool UseProxy = storage_traits<T>::use_proxy>
class Container {
 public:
  using value_type = T;
  using raw_value_type = typename storage_traits<T>::raw_value_type;

  using iterator = raw_value_type*;
  using const_iterator = const raw_value_type*;

  Container() = default;

  Container(const Container& other) : Container(other.begin(), other.end()) {}

  Container& operator=(const Container& other) {
    if (this == &other) return *this;
    Container tmp(other);
    std::swap(storage_, tmp.storage_);
    return *this;
  }

  Container(Container&&) noexcept = default;
  Container& operator=(Container&&) noexcept = default;

  Container(std::initializer_list<T> init) {
    std::ranges::transform(init, begin(), [](const T& v) {
      return static_cast<raw_value_type>(v);
    });
  }

  template <std::forward_iterator It>
    requires std::convertible_to<std::iter_value_t<It>, raw_value_type>
  Container(It first, It last) {
    assert(std::distance(first, last) == N);
    std::copy(first, last, begin());
  }

  // This constructor is used by interpret_as to take ownership of the buffer
  Container(raw_value_type* ptr) : storage_(ptr) {}

  template <typename F>
    requires requires(F& f, std::size_t i) {
      { std::invoke(f, i) } -> explicitly_convertible_to_concept<T>;
    }
  Container(F&& f) {
    std::size_t i = 0;
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f, i++));
    });
  }

  template <typename F>
    requires(
        !requires(F& f, std::size_t i) {
          { std::invoke(f, i) } -> explicitly_convertible_to_concept<T>;
        } &&
        requires(F& f) {
          { std::invoke(f) } -> explicitly_convertible_to_concept<T>;
        })
  Container(F&& f) {
    std::ranges::generate(begin(), end(), [&] {
      return static_cast<raw_value_type>(std::invoke(f));
    });
  }

  decltype(auto) operator[](size_t i) noexcept {
    if constexpr (UseProxy) {
      return Proxy<Derived>(storage_.get() + i);
    } else {
      return static_cast<value_type&>(storage_[i]);
    }
  }

  decltype(auto) operator[](size_t i) const noexcept {
    if constexpr (UseProxy) {
      return static_cast<value_type>(storage_[i]);
    } else {
      return storage_[i];
    }
  }

  iterator begin() noexcept { return storage_.get(); }
  iterator end() noexcept { return storage_.get() + N; }

  const_iterator begin() const noexcept { return storage_.get(); }
  const_iterator end() const noexcept { return storage_.get() + N; }

  raw_value_type* data() noexcept { return storage_.get(); }
  const raw_value_type* data() const noexcept { return storage_.get(); }

  static constexpr size_t size() { return N; }

 protected:
  raw_value_type* release_buffer() { return storage_.release(); }
  std::unique_ptr<raw_value_type[]> storage_ =
      std::make_unique<raw_value_type[]>(N);
};

#endif