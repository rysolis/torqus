// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_NOISE_TRACKER_HPP
#define TFHE_UTILITY_NOISE_TRACKER_HPP

#include <unordered_map>

class NoiseTracker : public NoiseTrackerInterface {
 public:
  void do_update(const void* key, double bound) override { db_[key] = bound; }

  double do_get(const void* key) const override {
    auto it = db_.find(key);
    assert(it != db_.end());
    return it->second;
  }

 private:
  std::unordered_map<const void*, double> db_;
};

#endif
