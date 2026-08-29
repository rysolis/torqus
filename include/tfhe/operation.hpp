// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_OPERATION_HPP
#define TFHE_OPERATION_HPP

// operation/evaluator.hpp is deliberately not included here: it pulls in
// utility/analysis/noise.hpp, which itself includes this file to see every
// op class's definition before specializing NoisePolicy for each -- adding
// evaluator.hpp here would make that a cycle. Include it directly if
// tracking-aware exec() is needed.
#include "tfhe/operation/bootstrap/blindrotate.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/operation/bootstrap/primitives/cmux.hpp"
#include "tfhe/operation/bootstrap/primitives/external_product.hpp"
#include "tfhe/operation/leveled/add.hpp"
#include "tfhe/operation/leveled/key_switch.hpp"
#include "tfhe/operation/leveled/sample_extract.hpp"
#include "tfhe/operation/leveled/sub.hpp"

#endif  // TFHE_OPERATION_HPP
