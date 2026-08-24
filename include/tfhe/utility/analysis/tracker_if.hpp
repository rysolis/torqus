// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_TRACKER_IF_HPP
#define TFHE_UTILITY_TRACKER_IF_HPP

#include <cmath>
#include <cstdint>
#include <unordered_map>

// Defined in variance_noise.hpp (needs boost::math, which production/wasm
// code must not pull in) -- declaration only, so confidence_threshold below
// still compiles here.
double z99_for_max_of(uint32_t coefficient_count);

class NoiseTrackerInterface {
 public:
  ~NoiseTrackerInterface() = default;

  template <typename Ciphertext>
  void update(const Ciphertext& ct, double bound) {
    do_update(key(ct), bound);
  }

  template <typename Ciphertext>
  double get(const Ciphertext& ct) const {
    return do_get(key(ct));
  }

  // For a variance tracker, the 99% confidence threshold on infinity_norm.
  template <typename Ciphertext>
  double confidence_threshold(const Ciphertext& ct,
                              uint32_t coefficient_count) const {
    return z99_for_max_of(coefficient_count) * std::sqrt(get(ct));
  }

 private:
  template <typename Ciphertext>
  static const void* key(const Ciphertext& ct) {
    return ct.identity();
  }

  virtual double do_get(const void*) const = 0;
  virtual void do_update(const void*, double) = 0;
};

inline NoiseTrackerInterface* get_noise_tracker_if();
inline NoiseTrackerInterface* get_key_noise_tracker_if();
inline NoiseTrackerInterface* get_variance_tracker_if();
inline NoiseTrackerInterface* get_key_variance_tracker_if();
inline void set_noise_tracker_if(NoiseTrackerInterface* impl);
inline void set_key_noise_tracker_if(NoiseTrackerInterface* impl);
inline void set_variance_tracker_if(NoiseTrackerInterface* impl);
inline void set_key_variance_tracker_if(NoiseTrackerInterface* impl);

#include "tracker/tracker_if_impl.hpp"

#endif