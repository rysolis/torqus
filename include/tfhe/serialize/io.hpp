// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_IO_HPP
#define TFHE_SERIALIZE_IO_HPP

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "tfhe/serialize/buffer.hpp"
#include "tfhe/serialize/serde.hpp"

namespace serialize {

// write/read actually write/read a destination (a Writer/Reader already
// in hand, or a std::ostream/std::istream -- a real file, so a huge key
// never needs a second, fully-materialized copy of itself as a byte
// buffer just to reach disk).
template <typename T>
void write(Writer& w, const T& value) {
  Serde<T>::serialize(w, value);
}

template <typename T>
T read(Reader& r) {
  return Serde<T>::deserialize(r);
}

template <typename T>
void write(std::ostream& os, const T& value) {
  Writer w(os);
  Serde<T>::serialize(w, value);
}

template <typename T>
T read(std::istream& is) {
  Reader r(is);
  return Serde<T>::deserialize(r);
}

namespace detail {

// The two ends of "an in-memory std::ostringstream/istringstream, as a
// std::vector<std::byte>" that to_bytes/from_bytes each need one
// direction of -- pulled out so neither buries that conversion inside
// its own actual job (driving Serde<T> against a stream).

inline std::vector<std::byte> extract_bytes(std::ostringstream&& oss) {
  std::string s = std::move(oss).str();
  const auto* begin = reinterpret_cast<const std::byte*>(s.data());
  return std::vector<std::byte>(begin, begin + s.size());
}

inline std::istringstream as_istringstream(
    const std::vector<std::byte>& bytes) {
  return std::istringstream(
      std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()),
      std::ios::binary);
}

}  // namespace detail

// to_bytes/from_bytes don't write/read anywhere -- no destination, just a
// value in and a value (a std::vector<std::byte>) back out, via an
// in-memory std::ostringstream/istringstream under the hood. The one-shot
// convenience for the shape a REST call would actually send/receive --
// one extra in-memory copy (string <-> vector<byte>), fine for that size
// of value, not the huge-key path write/read above are for.
template <typename T>
std::vector<std::byte> to_bytes(const T& value) {
  std::ostringstream oss(std::ios::binary);
  Writer w(oss);
  Serde<T>::serialize(w, value);
  return detail::extract_bytes(std::move(oss));
}

template <typename T>
T from_bytes(const std::vector<std::byte>& bytes) {
  std::istringstream iss = detail::as_istringstream(bytes);
  Reader r(iss);
  return Serde<T>::deserialize(r);
}

}  // namespace serialize

#endif  // TFHE_SERIALIZE_IO_HPP
