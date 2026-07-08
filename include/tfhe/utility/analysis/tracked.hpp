// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_TRACKED_HPP
#define TFHE_UTILITY_TRACKED_HPP

#include <cstdint>
#include <optional>
#include <random>

#include "tfhe/concept/tfhe.hpp"
#include "tfhe/operation/external_product.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/noise.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"

template <typename Operation>
class TrackedEvaluator {
 public:
  template <typename... Args>
  static auto exec(const Args&... args) {
    NoiseTrackerInterface* tracker = get_noise_tracker_if();
    auto res = Operation::exec(args...);
    double bound = NoisePolicy<Operation>::compute(tracker, args...);

#ifndef NDEBUG
    if (bound >= 0.25) {
      std::cerr << "error_bound = " << bound << '\n';
    }
    assert(bound < 0.25);
#endif

    tracker->update(res, bound);
    return res;
  }
};

template <typename Cryptor, typename Engine = std::mt19937>
class TrackedCryptor {
 public:
  using params = typename Cryptor::params_type;
  TrackedCryptor() = default;
  explicit TrackedCryptor(std::shared_ptr<UInt::raw_value_type[]> s,
                          Engine& eng)
      : cryptor_(std::in_place, s, eng) {}

  template <typename Plaintext>
  auto encrypt(const Plaintext& pt) {
    assert(cryptor_.has_value());
    NoiseTrackerInterface* tracker = get_noise_tracker_if();
    auto res = cryptor_->encrypt(pt);
    double bound = 0.0;  // TODO: use params to compute bound
    tracker->update(res, bound);
    return res;
  }

  template <typename Ciphertext>
  auto decrypt(const Ciphertext& ct) {
    assert(cryptor_.has_value());
    return cryptor_->decrypt(ct);
  }

 private:
  std::optional<Cryptor> cryptor_;
};

#endif