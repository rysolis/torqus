// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_SERDE_HPP
#define TFHE_SERIALIZE_SERDE_HPP

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

#include "tfhe/serialize/buffer.hpp"
#include "tfhe/serialize/descriptor.hpp"

namespace serialize {

// Serde<T> is the extension point every serializable type specializes:
// Serde<T>::serialize(Writer&, const T&) and Serde<T>::deserialize(Reader&)
// -> T -- named for what Serde itself stands for (SERialize/DEserialize),
// distinct from the write/read entry points below (which are about the
// destination/source -- a Writer/Reader, a stream, a byte vector -- not
// about T itself). The primary template is deliberately left undefined --
// trying to serialize a type nobody has written a specialization for is a
// compile error rather than a silent no-op.
//
// Each specialization also provides byte_size() (the fixed length of its
// raw encoding) and descriptor() (its type descriptor, see descriptor.hpp).
template <typename T>
struct Serde;

// Every scalar primitive this library has (Torus, ModTorus<QBit>, ModInt,
// ...) wraps exactly one raw_value_type and is constructible back from it
// (see each type's own header under primitive/), so this single
// specialization covers all of them without per-type duplication. UInt is
// deliberately excluded (no .value()): it never appears inside a
// ciphertext/key's own fields, only as a transient secret-bit scalar
// during key generation, so it has nothing that needs to cross the wire.
template <typename T>
concept scalar_serializable =
    requires(const T& t) {
      typename T::raw_value_type;
      { t.value() } -> std::same_as<typename T::raw_value_type>;
    } &&
    (std::same_as<typename T::raw_value_type, uint32_t> ||
     std::same_as<typename T::raw_value_type, uint64_t> ||
     std::same_as<typename T::raw_value_type, double>) &&
    std::constructible_from<T, typename T::raw_value_type>;

template <scalar_serializable T>
struct Serde<T> {
  using Raw = typename T::raw_value_type;

  static constexpr std::size_t byte_size() { return sizeof(Raw); }
  static constexpr auto descriptor() {
    return detail::scalar_descriptor<T>::value;
  }

  static void serialize(Writer& w, const T& value) {
    if constexpr (std::same_as<Raw, double>) {
      w.write(std::bit_cast<uint64_t>(value.value()));
    } else {
      w.write(value.value());  // Raw is uint32_t or uint64_t; overload
                               // resolution picks the matching write().
    }
  }

  static T deserialize(Reader& r) {
    if constexpr (std::same_as<Raw, double>) {
      uint64_t bits;
      r.read(bits);
      return T(std::bit_cast<double>(bits));
    } else {
      Raw v;
      r.read(v);
      return T(v);
    }
  }
};

namespace detail {

// A Vector/Poly of scalars whose in-memory buffer already is the wire
// format (little-endian raw values), so it can be copied in one call
// instead of one stream call per element.
template <typename C>
concept bulk_serializable =
    std::endian::native == std::endian::little &&
    scalar_serializable<typename C::value_type> &&
    std::same_as<typename C::raw_value_type,
                 typename C::value_type::raw_value_type>;

template <typename C>
void serialize_elements(Writer& w, const C& c) {
  using T = typename C::value_type;
  if constexpr (bulk_serializable<C>) {
    w.write(std::as_bytes(std::span(c.data(), c.size())));
  } else {
    for (uint32_t i = 0; i < c.size(); ++i) {
      Serde<T>::serialize(w, c[i]);
    }
  }
}

template <typename C>
C deserialize_elements(Reader& r) {
  using T = typename C::value_type;
  if constexpr (bulk_serializable<C>) {
    C c;
    auto* d = c.data();
    r.read(std::as_writable_bytes(std::span(d, c.size())));
    // Same normalization T(raw) applies on the per-element path, so a
    // corrupt stream can't leave out-of-range raw values behind.
    for (uint32_t i = 0; i < c.size(); ++i) {
      d[i] = T(d[i]).value();
    }
    return c;
  } else {
    return C([&] { return Serde<T>::deserialize(r); });
  }
}

}  // namespace detail

// Vector<T, Size> / Poly<T, Size>: Size copies of T back to back, each
// recursing through Serde<T>; a buffer of scalars is copied in one call.
template <typename T, uint32_t Size>
struct Serde<Vector<T, Size>> {
  static constexpr std::size_t byte_size() {
    return Size * Serde<T>::byte_size();
  }
  static constexpr auto descriptor() {
    return detail::concat(detail::head(TypeTag::Vector, Size),
                          Serde<T>::descriptor());
  }

  static void serialize(Writer& w, const Vector<T, Size>& v) {
    detail::serialize_elements(w, v);
  }

  static Vector<T, Size> deserialize(Reader& r) {
    return detail::deserialize_elements<Vector<T, Size>>(r);
  }
};

template <typename T, uint32_t Size>
struct Serde<Poly<T, Size>> {
  static constexpr std::size_t byte_size() {
    return Size * Serde<T>::byte_size();
  }
  static constexpr auto descriptor() {
    return detail::concat(detail::head(TypeTag::Poly, Size),
                          Serde<T>::descriptor());
  }

  static void serialize(Writer& w, const Poly<T, Size>& v) {
    detail::serialize_elements(w, v);
  }

  static Poly<T, Size> deserialize(Reader& r) {
    return detail::deserialize_elements<Poly<T, Size>>(r);
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_SERDE_HPP
