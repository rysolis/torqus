// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_SECRET_HOLDER_HPP
#define TFHE_UTILITY_SECRET_HOLDER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>

// Best-effort swap protection for the secret buffer below -- see
// detail::LockedSecretDeleter. Guard style follows
// algebra/detail/simd_lane.hpp's __ARM_NEON guard.
#if defined(__unix__) || defined(__APPLE__)
#define TORQUS_SECRET_MLOCK_ENABLED 1
#include <sys/mman.h>
#elif defined(_WIN32)
#define TORQUS_SECRET_VIRTUALLOCK_ENABLED 1
#include <windows.h>
#endif

#include "primitive/uint.hpp"

#include "algebra/poly.hpp"

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/params.hpp"

namespace detail {

// shared_ptr's own refcounting already tells us exactly when the last
// SecretHolder sharing this buffer (see the copy constructor relied on by
// Runtime's cross-Params constructor) goes away -- this deleter only needs
// to run then: zero the bits (through a volatile pointer, so the write
// can't be optimized away as dead) before releasing the memory, and drop
// the best-effort swap lock taken when the buffer was allocated.
template <typename T>
struct LockedSecretDeleter {
  std::size_t count;

  void operator()(T* p) const noexcept {
    volatile T* zeroing = p;
    for (std::size_t i = 0; i < count; ++i) {
      zeroing[i] = T{0};
    }
#if defined(TORQUS_SECRET_MLOCK_ENABLED)
    munlock(p, count * sizeof(T));
#elif defined(TORQUS_SECRET_VIRTUALLOCK_ENABLED)
    VirtualUnlock(p, count * sizeof(T));
#endif
    delete[] p;
  }
};

// Locking failure (e.g. hitting RLIMIT_MEMLOCK on Linux) is deliberately
// not fatal here -- it only means this particular buffer isn't swap-proof;
// LockedSecretDeleter zeroes it on release regardless.
template <typename T>
std::shared_ptr<T[]> make_locked_secret(std::size_t count) {
  T* raw = new T[count]();
#if defined(TORQUS_SECRET_MLOCK_ENABLED)
  mlock(raw, count * sizeof(T));
#elif defined(TORQUS_SECRET_VIRTUALLOCK_ENABLED)
  VirtualLock(raw, count * sizeof(T));
#endif
  return std::shared_ptr<T[]>(raw, LockedSecretDeleter<T>{count});
}

}  // namespace detail

template <uint32_t SecretSize>
class SecretHolder {
 public:
  using raw_value_type = UInt::raw_value_type;

  template <typename Engine>
    requires std::uniform_random_bit_generator<Engine>
  explicit SecretHolder(Engine& eng)
      : secret_(detail::make_locked_secret<raw_value_type>(SecretSize)) {
    std::uniform_int_distribution<raw_value_type> dist{0, 1};
    std::generate(secret_.get(), secret_.get() + SecretSize,
                  [&] { return dist(eng); });
  }

  const raw_value_type* get() const noexcept { return secret_.get(); }
  std::shared_ptr<raw_value_type[]> shared_get() const noexcept {
    return secret_;
  }

  static constexpr uint32_t size() { return SecretSize; }

 private:
  std::shared_ptr<raw_value_type[]> secret_;
};

#endif  // TFHE_UTILITY_SECRET_HOLDER_HPP