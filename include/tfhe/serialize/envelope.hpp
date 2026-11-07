// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_ENVELOPE_HPP
#define TFHE_SERIALIZE_ENVELOPE_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <span>
#include <sstream>
#include <string>

#include "tfhe/serialize/buffer.hpp"
#include "tfhe/serialize/error.hpp"
#include "tfhe/serialize/serde.hpp"

namespace serialize {

// Opt-in wrapper for torqus-to-torqus exchange that checks the exact type,
// including what length alone can't (see serialized_size). The raw format
// (write/to_bytes) stays header-less for interoperability; the body here is
// that raw format byte for byte, so stripping the envelope yields it.
//
//   magic       4 bytes  "TQSE"
//   version     u32      kEnvelopeVersion
//   desc_len    u32
//   descriptor  u32 x desc_len   (Serde<T>::descriptor())
//   body        raw encoding of the value
inline constexpr std::array<std::byte, 4> kEnvelopeMagic{
    std::byte{'T'}, std::byte{'Q'}, std::byte{'S'}, std::byte{'E'}};
inline constexpr uint32_t kEnvelopeVersion = 1;

template <typename T>
void write_enveloped(std::ostream& os, const T& value) {
  constexpr auto desc = Serde<T>::descriptor();
  Writer w(os);
  w.write(std::span<const std::byte>(kEnvelopeMagic));
  w.write(kEnvelopeVersion);
  w.write(static_cast<uint32_t>(desc.size()));
  for (uint32_t d : desc) {
    w.write(d);
  }
  Serde<T>::serialize(w, value);
}

namespace detail {

template <std::size_t K>
std::string descriptor_to_string(const std::array<uint32_t, K>& desc) {
  std::ostringstream oss;
  oss << std::hex << '{';
  for (std::size_t i = 0; i < K; ++i) {
    oss << (i == 0 ? "" : ", ") << "0x" << desc[i];
  }
  oss << '}';
  return oss.str();
}

}  // namespace detail

template <typename T>
T read_enveloped(std::istream& is) {
  constexpr auto expected = Serde<T>::descriptor();
  Reader r(is);

  std::array<std::byte, 4> magic;
  r.read(std::span<std::byte>(magic));
  if (magic != kEnvelopeMagic) {
    throw format_error("serialize::read_enveloped: bad magic");
  }

  uint32_t version;
  r.read(version);
  if (version != kEnvelopeVersion) {
    throw format_error("serialize::read_enveloped: unsupported version " +
                       std::to_string(version));
  }

  uint32_t len;
  r.read(len);
  if (len != expected.size()) {
    throw format_error(
        "serialize::read_enveloped: descriptor mismatch: expected length " +
        std::to_string(expected.size()) + ", got " + std::to_string(len));
  }
  std::array<uint32_t, expected.size()> actual;
  for (uint32_t& d : actual) {
    r.read(d);
  }
  if (actual != expected) {
    throw format_error(
        "serialize::read_enveloped: descriptor mismatch: "
        "expected " +
        detail::descriptor_to_string(expected) + ", got " +
        detail::descriptor_to_string(actual));
  }

  return Serde<T>::deserialize(r);
}

}  // namespace serialize

#endif  // TFHE_SERIALIZE_ENVELOPE_HPP
