// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

// Byte-buffer (de)serialization for every ciphertext/key structure this
// library hands between roles that, in a real deployment, may live on
// separate machines (e.g. the an example application protocol's Coordinator/Client/
// Downstream/Aggregator -- see example/protocol/*.hpp and tfhe/transport.hpp).
//
// Usage: serialize::to_bytes(value) -> std::vector<std::byte>,
// serialize::from_bytes<T>(bytes) -> T. write/read against an existing
// Writer/Reader pack several values into one buffer. See serialize/
// serde.hpp for the extension point (Serde<T>) new serializable types
// hook into.

#ifndef TFHE_SERIALIZE_HPP
#define TFHE_SERIALIZE_HPP

#include "tfhe/serialize/bootstrap_key.hpp"
#include "tfhe/serialize/buffer.hpp"
#include "tfhe/serialize/key_switch_key.hpp"
#include "tfhe/serialize/public_key.hpp"
#include "tfhe/serialize/serde.hpp"
#include "tfhe/serialize/tlwe.hpp"
#include "tfhe/serialize/trgsw.hpp"
#include "tfhe/serialize/trlwe.hpp"

#endif  // TFHE_SERIALIZE_HPP
