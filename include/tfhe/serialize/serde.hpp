// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_SERDE_HPP
#define TFHE_SERIALIZE_SERDE_HPP

#include <bit>
#include <concepts>
#include <cstdint>

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

#include "tfhe/serialize/buffer.hpp"

namespace serialize {

// Serde<T> is the extension point every serializable type specializes:
// Serde<T>::serialize(Writer&, const T&) and Serde<T>::deserialize(Reader&)
// -> T -- named for what Serde itself stands for (SERialize/DEserialize),
// distinct from the write/read entry points below (which are about the
// destination/source -- a Writer/Reader, a stream, a byte vector -- not
// about T itself). The primary template is deliberately left undefined --
// trying to serialize a type nobody has written a specialization for is a
// compile error rather than a silent no-op.
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
      return T(std::bit_cast<double>(r.read<uint64_t>()));
    } else {
      return T(r.read<Raw>());
    }
  }
};

// Vector<T, Size> / Poly<T, Size>: Size copies of T back to back, each
// recursing through Serde<T> -- one specialization handles every nesting
// depth this library actually has (Vector<Torus,n>, Vector<TRLWE<Torus,N>,
// 2l>, Vector<Vector<TLWE<Torus,n>,t>,m>, ...) uniformly.
template <typename T, uint32_t Size>
struct Serde<Vector<T, Size>> {
  static void serialize(Writer& w, const Vector<T, Size>& v) {
    for (uint32_t i = 0; i < Size; ++i) {
      Serde<T>::serialize(w, v[i]);
    }
  }

  static Vector<T, Size> deserialize(Reader& r) {
    return Vector<T, Size>([&] { return Serde<T>::deserialize(r); });
  }
};

template <typename T, uint32_t Size>
struct Serde<Poly<T, Size>> {
  static void serialize(Writer& w, const Poly<T, Size>& v) {
    for (uint32_t i = 0; i < Size; ++i) {
      Serde<T>::serialize(w, v[i]);
    }
  }

  static Poly<T, Size> deserialize(Reader& r) {
    return Poly<T, Size>([&] { return Serde<T>::deserialize(r); });
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_SERDE_HPP
