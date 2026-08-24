// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_NOISE_TRACKER_HPP
#define TFHE_UTILITY_NOISE_TRACKER_HPP

#include <cassert>
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

class KeyNoiseTracker : public NoiseTrackerInterface {
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

// Same shape as NoiseTracker/KeyNoiseTracker above -- distinct types only so
// get_tracker_if<T>() (tracker_if_impl.hpp) gives each role its own
// singleton. Values stored here are variances (Var(Err(ct))), never the
// worst-case bounds NoiseTracker/KeyNoiseTracker carry -- see
// variance_noise.hpp's VarianceNoisePolicy, the only intended writer/reader.
class VarianceTracker : public NoiseTrackerInterface {
 public:
  void do_update(const void* key, double variance) override {
    db_[key] = variance;
  }

  double do_get(const void* key) const override {
    auto it = db_.find(key);
    assert(it != db_.end());
    return it->second;
  }

 private:
  std::unordered_map<const void*, double> db_;
};

class KeyVarianceTracker : public NoiseTrackerInterface {
 public:
  void do_update(const void* key, double variance) override {
    db_[key] = variance;
  }

  double do_get(const void* key) const override {
    auto it = db_.find(key);
    assert(it != db_.end());
    return it->second;
  }

 private:
  std::unordered_map<const void*, double> db_;
};

#endif
