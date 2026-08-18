// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_TRANSPORT_HPP
#define TFHE_TRANSPORT_HPP

#include <utility>

#include "tfhe/serialize.hpp"

// A Transport decides what shape a value takes crossing a hand-off
// boundary between roles in example/protocol/*.hpp, and how to cross it. Every
// hand-off point is written once, generic over Transport, switched by
// template parameter alone:
//
//   - DirectTransport: the value itself, passed through untouched -- same
//     cost as passing the live C++ object directly, no encode/decode.
//   - SerializedTransport (the default): serialize::to_bytes'd/from_bytes'd
//     (tfhe/serialize.hpp) -- the shape the same hand-off would need to
//     take crossing a real network, since Protocol's roles are meant to be
//     splittable across machines (see example/protocol.hpp).
struct DirectTransport {
  template <typename T>
  using wire_type = T;

  template <typename T>
  static T send(T value) {
    return value;
  }

  template <typename T>
  static T receive(T value) {
    return value;
  }
};

struct SerializedTransport {
  template <typename T>
  using wire_type = std::vector<std::byte>;

  template <typename T>
  static std::vector<std::byte> send(const T& value) {
    return serialize::to_bytes(value);
  }

  template <typename T>
  static T receive(const std::vector<std::byte>& bytes) {
    return serialize::from_bytes<T>(bytes);
  }
};

#endif  // TFHE_TRANSPORT_HPP
