// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_TRACKER_IF_HPP
#define TFHE_UTILITY_TRACKER_IF_HPP

#include <cmath>
#include <cstdint>
#include <unordered_map>

// 99% two-sided threshold multiplier for max_i|X_i| over coefficient_count
// independent sigma-subgaussian samples, via the sub-Gaussian tail bound
// P(|X| > t) <= 2*exp(-t^2/(2*sigma^2)) (Assumption 3.11's own tail model --
// see the README's Bootstrap Noise Bounds section) combined with the
// Bonferroni bound P(max_i|X_i| > t) <= coefficient_count * P(|X| > t).
// Solving coefficient_count * 2*exp(-t^2/(2*sigma^2)) = 0.01 for t/sigma
// gives the multiplier below; reduces to ~3.255 at coefficient_count=1.
inline double subgaussian_bound_for_max_of(uint32_t coefficient_count) {
  double tail_probability = 0.005 / static_cast<double>(coefficient_count);
  return std::sqrt(-2.0 * std::log(tail_probability));
}

// Same 99% two-sided threshold multiplier, but under the (unproven) further
// simplification that the error sum is exactly Gaussian rather than merely
// sigma-subgaussian -- see the README's Bootstrap Noise Bounds section.
// Informational only, never the basis for a pass/fail check. Defined in
// variance_noise.hpp (needs boost::math, which production/wasm code must
// not pull in) -- declaration only, so callers here can still name it.
double gaussian_estimate_for_max_of(uint32_t coefficient_count);

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
    return subgaussian_bound_for_max_of(coefficient_count) * std::sqrt(get(ct));
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