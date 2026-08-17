// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_TRACKER_IF_IMPL_HPP
#define TFHE_UTILITY_TRACKER_IF_IMPL_HPP

#include "tfhe/utility/analysis/tracker/noise_tracker.hpp"

// forward declaration
class NoiseTrackerInterface;

inline NoiseTrackerInterface* instance = nullptr;

template <typename Tracker>
inline NoiseTrackerInterface* get_tracker_if() {
  [[maybe_unused]]
  static bool init = [] {
    if (!instance) {
      static Tracker tracker;
      instance = &tracker;
    }
    return true;
  }();

  return instance;
}

inline NoiseTrackerInterface* get_noise_tracker_if() {
  return get_tracker_if<NoiseTracker>();
}

inline NoiseTrackerInterface* get_key_noise_tracker_if() {
  return get_tracker_if<KeyNoiseTracker>();
}

inline void set_noise_tracker_if(NoiseTrackerInterface* impl) { instance = impl; }

#endif