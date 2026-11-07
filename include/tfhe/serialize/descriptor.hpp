// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_DESCRIPTOR_HPP
#define TFHE_SERIALIZE_DESCRIPTOR_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#include "primitive/modint.hpp"
#include "primitive/torus.hpp"

namespace serialize {

// A type descriptor is a flat std::array<uint32_t, K>: a TypeTag, that
// type's own dimensions, then its element type's descriptor. It lives
// only in the opt-in envelope (envelope.hpp), never in the raw format.
//
// Tag values are part of the envelope format: never renumber or reuse one.
// uint32_t because descriptors are written as u32 values.
enum class TypeTag : uint32_t {  // NOLINT(performance-enum-size)
  ModTorus = 1,
  DoubleTorus = 2,
  ModInt = 3,
  Vector = 16,
  Poly = 17,
  Tlwe = 32,
  Trlwe = 33,
  Trgsw = 34,
  BootstrapKey = 48,
  KeySwitchKey = 49,
  PublicKey = 50,
};

namespace detail {

template <std::size_t... Ns>
constexpr std::array<uint32_t, (Ns + ...)> concat(
    const std::array<uint32_t, Ns>&... parts) {
  std::array<uint32_t, (Ns + ...)> out{};
  std::size_t pos = 0;
  (
      [&] {
        for (uint32_t v : parts) {
          out[pos++] = v;
        }
      }(),
      ...);
  return out;
}

// Header of a descriptor: the tag followed by the type's own dimensions.
template <typename... Dims>
constexpr std::array<uint32_t, 1 + sizeof...(Dims)> head(TypeTag tag,
                                                         Dims... dims) {
  return {static_cast<uint32_t>(tag), static_cast<uint32_t>(dims)...};
}

// Undefined for a scalar nobody has given a descriptor, so putting one in
// an envelope is a compile error rather than an ambiguous descriptor.
template <typename T>
struct scalar_descriptor;

template <uint32_t QBit, typename Word>
struct scalar_descriptor<ModTorus<QBit, Word>> {
  static constexpr auto value = head(TypeTag::ModTorus, QBit, sizeof(Word));
};

template <>
struct scalar_descriptor<dbl::Torus> {
  static constexpr auto value = head(TypeTag::DoubleTorus);
};

template <uint64_t P, typename Word>
struct scalar_descriptor<ModInt<P, Word>> {
  static constexpr auto value =
      head(TypeTag::ModInt, P & 0xffffffffu, P >> 32, sizeof(Word));
};

}  // namespace detail

}  // namespace serialize

#endif  // TFHE_SERIALIZE_DESCRIPTOR_HPP
