// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_SECRET_HPP
#define TFHE_SERIALIZE_SECRET_HPP

#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>

#include "tfhe/serialize/buffer.hpp"
#include "tfhe/serialize/error.hpp"
#include "tfhe/utility/secret_holder.hpp"

namespace serialize {

// A secret key's encoding: each element (0 or 1) as a little-endian u32,
// in order, independent of TORQUS_TORUS_BITS. Unencrypted, as other FHE
// libraries save secret keys; encrypting it at rest is the application's
// job.
//
// These are named functions rather than a Serde<SecretHolder>, so a
// secret can never go out through to_bytes or a Transport by accident.
//
// To encrypt without leaving plaintext behind, use the span overloads
// with a locked, zeroed-on-free buffer (e.g. sodium_malloc), or the
// stream overloads with an encrypting std::streambuf. Going through
// std::ostringstream leaves unzeroed copies of the plaintext in freed
// heap memory.
template <uint32_t SecretLength>
constexpr std::size_t secret_size() {
  return std::size_t{SecretLength} * sizeof(uint32_t);
}

namespace detail {

inline uint32_t checked_secret_bit(uint32_t v) {
  if (v > 1) {
    throw format_error("serialize::read_secret: element is " +
                       std::to_string(v) + ", not 0 or 1");
  }
  return v;
}

}  // namespace detail

template <uint32_t SecretLength>
void write_secret(std::ostream& os, const SecretHolder<SecretLength>& holder) {
  Writer w(os);
  for (uint32_t i = 0; i < SecretLength; ++i) {
    w.write(static_cast<uint32_t>(holder.get()[i]));
  }
}

template <uint32_t SecretLength>
SecretHolder<SecretLength> read_secret(std::istream& is) {
  using Raw = typename SecretHolder<SecretLength>::raw_value_type;
  Reader r(is);
  return SecretHolder<SecretLength>(secret_fill, [&] {
    uint32_t v;
    r.read(v);
    return static_cast<Raw>(detail::checked_secret_bit(v));
  });
}

// out must be exactly secret_size<SecretLength>() bytes.
template <uint32_t SecretLength>
void write_secret(std::span<std::byte> out,
                  const SecretHolder<SecretLength>& holder) {
  if (out.size() != secret_size<SecretLength>()) {
    throw std::invalid_argument(
        "serialize::write_secret: buffer is " + std::to_string(out.size()) +
        " bytes, expected " + std::to_string(secret_size<SecretLength>()));
  }
  for (uint32_t i = 0; i < SecretLength; ++i) {
    const auto v = static_cast<uint32_t>(holder.get()[i]);
    for (std::size_t k = 0; k < sizeof(uint32_t); ++k) {
      out[i * sizeof(uint32_t) + k] = static_cast<std::byte>(v >> (8 * k));
    }
  }
}

// Rejects input whose length isn't exactly secret_size<SecretLength>().
template <uint32_t SecretLength>
SecretHolder<SecretLength> read_secret(std::span<const std::byte> in) {
  using Raw = typename SecretHolder<SecretLength>::raw_value_type;
  if (in.size() != secret_size<SecretLength>()) {
    throw format_error("serialize::read_secret: expected " +
                       std::to_string(secret_size<SecretLength>()) +
                       " bytes, got " + std::to_string(in.size()));
  }
  std::size_t pos = 0;
  return SecretHolder<SecretLength>(secret_fill, [&] {
    uint32_t v = 0;
    for (std::size_t k = 0; k < sizeof(uint32_t); ++k) {
      v |= static_cast<uint32_t>(in[pos++]) << (8 * k);
    }
    return static_cast<Raw>(detail::checked_secret_bit(v));
  });
}

}  // namespace serialize

#endif  // TFHE_SERIALIZE_SECRET_HPP
