// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#include "tfhe/utility/analysis/tracker/noise_tracker.hpp"

#ifndef TFHE_UTILITY_TRACKER_IF_IMPL_HPP
#define TFHE_UTILITY_TRACKER_IF_IMPL_HPP

// forward declaration
class NoiseTrackerInterface;

inline NoiseTrackerInterface* instance = nullptr;

inline NoiseTrackerInterface* get_noise_tracker_if() {
  // "the actual value does not matter"
  [[maybe_unused]]
  static bool init = []() {
    if (!instance) {
      static NoiseTracker nt_;
      instance = &nt_;
    }
    return true;
  }();
  return instance;
}

inline void set_noise_tracker_if(NoiseTrackerInterface* itf) { instance = itf; }

#endif