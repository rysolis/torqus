#ifndef PROXY_HPP
#define PROXY_HPP

#include <iostream>

template <typename T, uint32_t N>
class Poly;

template <typename T, uint32_t N>
class Poly<T, N>::Proxy {
 public:
  using value_type = T;
  using raw_value_type = typename T::raw_value_type;

  // Disable copying to prevent expressions such as:
  // auto x = p[1];
  Proxy(const Proxy&) = delete;
  Proxy& operator=(const Proxy& other) = delete;

  explicit Proxy(raw_value_type* raw) : ptr_(raw) {}

  Proxy& operator=(value_type v) noexcept {
    *ptr_ = static_cast<raw_value_type>(v);
    return *this;
  }

  Proxy& operator+=(value_type rhs) noexcept {
    *ptr_ = static_cast<raw_value_type>(static_cast<value_type>(*ptr_) + rhs);
    return *this;
  }

  Proxy& operator-=(value_type rhs) noexcept {
    *ptr_ = static_cast<raw_value_type>(static_cast<value_type>(*ptr_) - rhs);
    return *this;
  }

  // Allow implicit conversion from Poly<T>::Proxy to T.
  constexpr operator value_type() const noexcept {
    return static_cast<value_type>(*ptr_);
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

#endif