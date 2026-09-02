// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_SERDE_HPP
#define TFHE_SERIALIZE_SERDE_HPP

#include <bit>
#include <concepts>
#include <cstdint>
#include <vector>

#include "algebra/poly.hpp"
#include "algebra/vector.hpp"

#include "tfhe/serialize/buffer.hpp"

namespace serialize {

// Serde<T> is the extension point every serializable type specializes:
// Serde<T>::write(Writer&, const T&) and Serde<T>::read(Reader&) -> T. The
// primary template is deliberately left undefined -- trying to serialize a
// type nobody has written a specialization for is a compile error rather
// than a silent no-op.
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

  static void write(Writer& w, const T& value) {
    if constexpr (std::same_as<Raw, double>) {
      w.write_u64(std::bit_cast<uint64_t>(value.value()));
    } else if constexpr (std::same_as<Raw, uint64_t>) {
      w.write_u64(value.value());
    } else {
      w.write_u32(value.value());
    }
  }

  static T read(Reader& r) {
    if constexpr (std::same_as<Raw, double>) {
      return T(std::bit_cast<double>(r.read_u64()));
    } else if constexpr (std::same_as<Raw, uint64_t>) {
      return T(r.read_u64());
    } else {
      return T(r.read_u32());
    }
  }
};

// Vector<T, Size> / Poly<T, Size>: Size copies of T back to back, each
// recursing through Serde<T> -- one specialization handles every nesting
// depth this library actually has (Vector<Torus,n>, Vector<TRLWE<Torus,N>,
// 2l>, Vector<Vector<TLWE<Torus,n>,t>,m>, ...) uniformly.
template <typename T, uint32_t Size>
struct Serde<Vector<T, Size>> {
  static void write(Writer& w, const Vector<T, Size>& v) {
    for (uint32_t i = 0; i < Size; ++i) {
      Serde<T>::write(w, v[i]);
    }
  }

  static Vector<T, Size> read(Reader& r) {
    return Vector<T, Size>([&] { return Serde<T>::read(r); });
  }
};

template <typename T, uint32_t Size>
struct Serde<Poly<T, Size>> {
  static void write(Writer& w, const Poly<T, Size>& v) {
    for (uint32_t i = 0; i < Size; ++i) {
      Serde<T>::write(w, v[i]);
    }
  }

  static Poly<T, Size> read(Reader& r) {
    return Poly<T, Size>([&] { return Serde<T>::read(r); });
  }
};

// Public entry points: write/read work against an existing Writer/Reader
// (e.g. to pack several values into one buffer); to_bytes/from_bytes are
// the one-shot convenience wrapping a single value in its own buffer, the
// shape a REST call would actually send/receive.
template <typename T>
void write(Writer& w, const T& value) {
  Serde<T>::write(w, value);
}

template <typename T>
T read(Reader& r) {
  return Serde<T>::read(r);
}

template <typename T>
std::vector<std::byte> to_bytes(const T& value) {
  Writer w;
  write(w, value);
  return w.release();
}

template <typename T>
T from_bytes(const std::vector<std::byte>& bytes) {
  Reader r(bytes);
  return read<T>(r);
}

}  // namespace serialize

#endif  // TFHE_SERIALIZE_SERDE_HPP
