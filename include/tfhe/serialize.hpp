// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

// Byte-buffer (de)serialization for every ciphertext/key structure this
// library hands between roles that, in a real deployment, may live on
// separate machines -- e.g. a multi-party protocol built downstream on top
// of this library, where each party runs on its own host (see
// tfhe/transport.hpp).
//
// Usage: serialize::write(value) -> std::vector<std::byte>,
// serialize::read<T>(bytes) -> T. Both are also overloaded on a
// Writer&/Reader& (to pack several values into one buffer) or a
// std::ostream&/std::istream& (a real file, with no second
// fully-materialized byte-buffer copy of a huge value). See serialize/
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
