// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_TRACKER_IF_HPP
#define TFHE_UTILITY_TRACKER_IF_HPP

#include <unordered_map>

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

 private:
  template <typename Ciphertext>
  static const void* key(const Ciphertext& ct) {
    return ct.identity();
  }

  virtual double do_get(const void*) const = 0;
  virtual void do_update(const void*, double) = 0;
};

inline NoiseTrackerInterface* get_noise_tracker_if();
inline void set_noise_tracker_if(NoiseTrackerInterface* persistenfce);

#include "tracker/tracker_if_impl.hpp"

#endif