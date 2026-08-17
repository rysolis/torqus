// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef ALGEBRA_DETAIL_PROXY_HPP
#define ALGEBRA_DETAIL_PROXY_HPP

#include <iostream>

template <typename Container>
class Proxy {
 public:
  using value_type = Container::value_type;
  using raw_value_type = typename Container::raw_value_type;

  // Disable copying to prevent expressions such as:
  // auto x = p[1];
  Proxy(const Proxy&) = delete;
  Proxy& operator=(const Proxy& other) = delete;

  explicit Proxy(raw_value_type* raw) : ptr_(raw) {}

  // Allow implicit conversion from Poly<T>::Proxy to T.
  constexpr operator value_type() const noexcept {
    return static_cast<value_type>(*ptr_);
  }

  constexpr Proxy& operator=(const value_type& v) noexcept {
    *ptr_ = static_cast<raw_value_type>(v);
    return *this;
  }

  explicit constexpr operator raw_value_type() const noexcept { return *ptr_; }

  constexpr raw_value_type* data() noexcept { return ptr_; }

  friend std::ostream& operator<<(std::ostream& os, const Proxy& ref) {
    return os << static_cast<value_type>(ref);
  }

 private:
  // Using raw_value_type& as a member was considered,
  // but pointer semantics provide better flexibility
  // for assignment and object lifetime management.
  raw_value_type* ptr_;
};

template <class Container>
typename Container::value_type operator-(const Proxy<Container>& p) {
  return -static_cast<typename Container::value_type>(p);
}

template <class Container>
typename Container::value_type operator+(const Proxy<Container>& lhs,
                                         const Proxy<Container>& rhs) {
  return static_cast<typename Container::value_type>(lhs) +
         static_cast<typename Container::value_type>(rhs);
}

template <class Container>
typename Container::value_type operator-(const Proxy<Container>& lhs,
                                         const Proxy<Container>& rhs) {
  return static_cast<typename Container::value_type>(lhs) -
         static_cast<typename Container::value_type>(rhs);
}

#endif  // ALGEBRA_DETAIL_PROXY_HPP