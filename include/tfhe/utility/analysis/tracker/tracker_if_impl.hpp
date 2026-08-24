// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_TRACKER_IF_IMPL_HPP
#define TFHE_UTILITY_TRACKER_IF_IMPL_HPP

#include "tfhe/utility/analysis/tracker/noise_tracker.hpp"

// forward declaration
class NoiseTrackerInterface;

// Each Tracker type (NoiseTracker, KeyNoiseTracker, VarianceTracker,
// KeyVarianceTracker, ...) gets its own independent instance slot here --
// a single shared `instance` (as this used to be) would make whichever
// get_tracker_if<T>() runs first "claim" that one slot for every T, so a
// later get_tracker_if<OtherT>() would silently return the *first* T's
// object instead of its own: e.g. get_key_noise_tracker_if() and
// get_key_variance_tracker_if() would alias to the same underlying
// unordered_map, so a bound and a variance registered for the very same
// ciphertext identity would overwrite each other.
template <typename Tracker>
inline NoiseTrackerInterface*& tracker_slot() {
  static NoiseTrackerInterface* instance = nullptr;
  return instance;
}

template <typename Tracker>
inline NoiseTrackerInterface* get_tracker_if() {
  NoiseTrackerInterface*& instance = tracker_slot<Tracker>();
  if (!instance) {
    static Tracker tracker;
    instance = &tracker;
  }
  return instance;
}

template <typename Tracker>
inline void set_tracker_if(NoiseTrackerInterface* impl) {
  tracker_slot<Tracker>() = impl;
}

inline NoiseTrackerInterface* get_noise_tracker_if() {
  return get_tracker_if<NoiseTracker>();
}

inline NoiseTrackerInterface* get_key_noise_tracker_if() {
  return get_tracker_if<KeyNoiseTracker>();
}

inline NoiseTrackerInterface* get_variance_tracker_if() {
  return get_tracker_if<VarianceTracker>();
}

inline NoiseTrackerInterface* get_key_variance_tracker_if() {
  return get_tracker_if<KeyVarianceTracker>();
}

inline void set_noise_tracker_if(NoiseTrackerInterface* impl) {
  set_tracker_if<NoiseTracker>(impl);
}

inline void set_key_noise_tracker_if(NoiseTrackerInterface* impl) {
  set_tracker_if<KeyNoiseTracker>(impl);
}

inline void set_variance_tracker_if(NoiseTrackerInterface* impl) {
  set_tracker_if<VarianceTracker>(impl);
}

inline void set_key_variance_tracker_if(NoiseTrackerInterface* impl) {
  set_tracker_if<KeyVarianceTracker>(impl);
}

#endif