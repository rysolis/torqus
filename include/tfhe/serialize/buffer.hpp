// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_BUFFER_HPP
#define TFHE_SERIALIZE_BUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace serialize {

// Appends fixed-width integers to a growing byte buffer, always
// little-endian regardless of the host's native byte order -- so two
// machines with different endianness still agree on the wire format once
// this crosses a network.
class Writer {
 public:
  void write_u32(uint32_t v) {
    for (uint32_t i = 0; i < 4; ++i) {
      buf_.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xffu));
    }
  }

  void write_u64(uint64_t v) {
    for (uint32_t i = 0; i < 8; ++i) {
      buf_.push_back(static_cast<std::byte>((v >> (8 * i)) & 0xffull));
    }
  }

  const std::vector<std::byte>& bytes() const noexcept { return buf_; }
  std::vector<std::byte> release() noexcept { return std::move(buf_); }

 private:
  std::vector<std::byte> buf_;
};

// Reads back exactly what Writer wrote, in the same order -- bounds
// checked, so a truncated or corrupt buffer throws rather than reading
// past the end.
class Reader {
 public:
  explicit Reader(const std::vector<std::byte>& bytes) : bytes_(bytes) {}

  uint32_t read_u32() {
    require(4);
    uint32_t v = 0;
    for (uint32_t i = 0; i < 4; ++i) {
      v |= static_cast<uint32_t>(bytes_[pos_++]) << (8 * i);
    }
    return v;
  }

  uint64_t read_u64() {
    require(8);
    uint64_t v = 0;
    for (uint32_t i = 0; i < 8; ++i) {
      v |= static_cast<uint64_t>(bytes_[pos_++]) << (8 * i);
    }
    return v;
  }

 private:
  void require(size_t n) const {
    if (pos_ + n > bytes_.size()) {
      throw std::out_of_range("serialize::Reader: buffer underrun");
    }
  }

  const std::vector<std::byte>& bytes_;
  size_t pos_ = 0;
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_BUFFER_HPP
