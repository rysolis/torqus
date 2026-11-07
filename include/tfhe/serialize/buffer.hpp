// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_BUFFER_HPP
#define TFHE_SERIALIZE_BUFFER_HPP

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <span>
#include <stdexcept>

namespace serialize {

namespace detail {

template <size_t NumBytes>
struct uint_for_size;
template <>
struct uint_for_size<4> {
  using type = uint32_t;
};
template <>
struct uint_for_size<8> {
  using type = uint64_t;
};
template <size_t NumBytes>
using uint_for_size_t = typename uint_for_size<NumBytes>::type;

// Integer <-> byte-array conversion, always little-endian regardless of
// the host's native byte order -- kept separate from Writer/Reader below
// (which only know "write/read these bytes") so this half is plain,
// stream-free logic. One template covers both widths (NumBytes=4 ->
// uint32_t, NumBytes=8 -> uint64_t) since the loop itself doesn't care
// which. std::byte, not char: char only enters at Writer/Reader's own
// I/O boundary, where std::ostream::write/std::istream::read require it.

template <size_t NumBytes>
std::array<std::byte, NumBytes> encode(uint_for_size_t<NumBytes> v) {
  std::array<std::byte, NumBytes> buf;
  for (size_t i = 0; i < NumBytes; ++i) {
    buf[i] = static_cast<std::byte>((v >> (8 * i)) & 0xffu);
  }
  return buf;
}

template <size_t NumBytes>
uint_for_size_t<NumBytes> decode(const std::array<std::byte, NumBytes>& buf) {
  uint_for_size_t<NumBytes> v = 0;
  for (size_t i = 0; i < NumBytes; ++i) {
    v |= static_cast<uint_for_size_t<NumBytes>>(buf[i]) << (8 * i);
  }
  return v;
}

}  // namespace detail

// Writes exactly the bytes detail::encode produces to any std::ostream (a
// real file, an in-memory std::ostringstream, ...). Templated on v's own
// type (uint32_t vs uint64_t) rather than two differently-named methods,
// so a caller never has to spell out which width -- the argument already
// says so.
class Writer {
 public:
  explicit Writer(std::ostream& os) : os_(os) {}

  template <typename T>
    requires std::same_as<T, uint32_t> || std::same_as<T, uint64_t>
  void write(T v) {
    put(detail::encode<sizeof(T)>(v));
  }

  // Raw bytes as-is, no per-value encoding; the caller is responsible for
  // them already being in the wire format.
  void write_bytes(std::span<const std::byte> bytes) { put(bytes); }

 private:
  // The only place std::byte meets std::ostream's char-based API.
  void put(std::span<const std::byte> bytes) {
    os_.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
  }

  std::ostream& os_;
};

// Reads back exactly what Writer wrote, in the same order -- a truncated
// or corrupt stream throws rather than reading past the end. Unlike
// Writer::write, can't overload on a return type, so the width is an
// explicit template argument instead: read<uint32_t>()/read<uint64_t>().
class Reader {
 public:
  explicit Reader(std::istream& is) : is_(is) {}

  template <typename T>
    requires std::same_as<T, uint32_t> || std::same_as<T, uint64_t>
  T read() {
    std::array<std::byte, sizeof(T)> buf;
    get(buf);
    return detail::decode<sizeof(T)>(buf);
  }

  // Fills bytes exactly, throwing on underrun like read().
  void read_bytes(std::span<std::byte> bytes) { get(bytes); }

 private:
  // The only place std::byte meets std::istream's char-based API.
  void get(std::span<std::byte> bytes) {
    is_.read(reinterpret_cast<char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
    if (static_cast<size_t>(is_.gcount()) != bytes.size()) {
      throw std::out_of_range("serialize::Reader: buffer underrun");
    }
  }

  std::istream& is_;
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_BUFFER_HPP
