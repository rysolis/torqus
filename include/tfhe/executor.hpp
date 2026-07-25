// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_EXECUTOR_HPP
#define TFHE_EXECUTOR_HPP

#include "primitive/concept/torus.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/structure/ciphertext/tlwe.hpp"
#include "tfhe/structure/ciphertext/trgsw.hpp"
#include "tfhe/structure/ciphertext/trlwe.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"

namespace executor {
template <typename Result>
void update(const Result& res) {
  auto* tracker = get_noise_tracker_if();
  double bound = 0.0;  // TODO: use parameters to compute
  tracker->update(res, bound);
}
}  // namespace executor

template <typename Cryptor, typename... Feature>
class Executor {
 public:
  Executor() = default;
  explicit Executor(Cryptor& cryptor) : cryptor_(cryptor) {}

  template <typename Plaintext>
  auto encrypt(const Plaintext& pt) {
    auto res = cryptor_->encrypt(pt);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      executor::update(res);
    }
    return res;
  }

  template <typename Ciphertext>
  auto decrypt(const Ciphertext& ct) {
    return cryptor_->decrypt(ct);
  }

 private:
  std::optional<Cryptor> cryptor_;
};

#endif