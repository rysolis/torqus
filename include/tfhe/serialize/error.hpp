// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_SERIALIZE_ERROR_HPP
#define TFHE_SERIALIZE_ERROR_HPP

#include <stdexcept>

namespace serialize {

// Input that is well-formed bytes but not the shape the caller asked for
// (wrong length, wrong envelope magic/version/descriptor). A stream that
// simply ends early is std::out_of_range instead (see Reader).
class format_error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace serialize

#endif  // TFHE_SERIALIZE_ERROR_HPP
