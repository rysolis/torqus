// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TEST_SUPPORT_PARAMS_HPP
#define TEST_SUPPORT_PARAMS_HPP

#include "tfhe/params.hpp"

// The README's own documented 128-bit security row (see "Bootstrap Noise
// Bounds") -- the one real parameter set several runtime tests already
// re-spell identically rather than share.
struct ProductionParams {
  using Lwe = lwe_params<tlwe_core_params<ModTorus<32>, 630>, noise_params<15>>;
  using Rlwe =
      rlwe_params<trlwe_core_params<ModTorus<32>, 1024>, noise_params<25>>;
  using Decomp = dcp_params<16, 7>;
  using Kst = kst_params<2, 11>;
};

// A small, fast parameter set for tests that just need real (not trivial)
// logic to exercise quickly -- not tuned to any particular security level.
struct ToyParams {
  using Lwe = lwe_params<tlwe_core_params<ModTorus<16>, 4>>;
  using Rlwe = rlwe_params<trlwe_core_params<ModTorus<16>, 32>>;
  using Decomp = dcp_params<4, 6>;
  using Kst = kst_params<4, 6>;
};

#endif  // TEST_SUPPORT_PARAMS_HPP
