// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

// Byte-buffer (de)serialization for every ciphertext/key structure this
// library hands between roles that, in a real deployment, may live on
// separate machines -- e.g. a multi-party protocol built downstream on top
// of this library, where each party runs on its own host (see
// tfhe/transport.hpp).
//
// Usage: serialize::to_bytes(value) -> std::vector<std::byte>,
// serialize::from_bytes<T>(bytes) -> T. write(Writer&/std::ostream&, ...)/
// read<T>(Reader&/std::istream&) write/read an existing destination
// instead -- a real file, with no second fully-materialized byte-buffer
// copy of a huge value (see serialize/io.hpp). The raw format has no
// header; from_bytes checks the length, and write_enveloped/
// read_enveloped (serialize/envelope.hpp) additionally check the exact
// type; serialize::Raw/Enveloped (serialize/format.hpp) pick between
// them in one place. See serialize/serde.hpp for the extension point (Serde<T>)
// new serializable types hook into. The byte layout is specified in
// https://github.com/rysolis/torqus/wiki/Serialization-Format.

#ifndef TFHE_SERIALIZE_HPP
#define TFHE_SERIALIZE_HPP

#include "tfhe/serialize/bootstrap_key.hpp"
#include "tfhe/serialize/buffer.hpp"
#include "tfhe/serialize/descriptor.hpp"
#include "tfhe/serialize/envelope.hpp"
#include "tfhe/serialize/error.hpp"
#include "tfhe/serialize/format.hpp"
#include "tfhe/serialize/io.hpp"
#include "tfhe/serialize/key_switch_key.hpp"
#include "tfhe/serialize/public_key.hpp"
#include "tfhe/serialize/serde.hpp"
#include "tfhe/serialize/tlwe.hpp"
#include "tfhe/serialize/trgsw.hpp"
#include "tfhe/serialize/trlwe.hpp"

#endif  // TFHE_SERIALIZE_HPP
