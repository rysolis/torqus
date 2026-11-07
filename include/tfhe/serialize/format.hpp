// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_FORMAT_HPP
#define TFHE_SERIALIZE_FORMAT_HPP

#include <cstddef>
#include <istream>
#include <ostream>
#include <vector>

#include "tfhe/serialize/envelope.hpp"
#include "tfhe/serialize/io.hpp"

namespace serialize {

// The format as a type, so an application picks it in one place
// (`using Format = serialize::Enveloped;`) and every Format::write/read/
// to_bytes/from_bytes call follows. In code generic over Format, name the
// explicit-T members as Format::template read<T>(is).

// Raw format: write/read/to_bytes/from_bytes (io.hpp).
struct Raw {
  template <typename T>
  static void write(std::ostream& os, const T& value) {
    serialize::write(os, value);
  }

  template <typename T>
  static T read(std::istream& is) {
    return serialize::read<T>(is);
  }

  template <typename T>
  static std::vector<std::byte> to_bytes(const T& value) {
    return serialize::to_bytes(value);
  }

  template <typename T>
  static T from_bytes(const std::vector<std::byte>& bytes) {
    return serialize::from_bytes<T>(bytes);
  }
};

// Envelope: the *_enveloped functions (envelope.hpp).
struct Enveloped {
  template <typename T>
  static void write(std::ostream& os, const T& value) {
    serialize::write_enveloped(os, value);
  }

  template <typename T>
  static T read(std::istream& is) {
    return serialize::read_enveloped<T>(is);
  }

  template <typename T>
  static std::vector<std::byte> to_bytes(const T& value) {
    return serialize::to_bytes_enveloped(value);
  }

  template <typename T>
  static T from_bytes(const std::vector<std::byte>& bytes) {
    return serialize::from_bytes_enveloped<T>(bytes);
  }
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_FORMAT_HPP
