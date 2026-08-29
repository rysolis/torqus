// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_EVALUATOR_HPP
#define TFHE_EVALUATOR_HPP

#include "tfhe/feature.hpp"
#include "tfhe/utility/analysis/noise.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"
#include "tfhe/utility/analysis/variance_noise.hpp"

namespace tfhe::operation {

namespace evaluator {

template <typename Op, typename Result, typename... Args>
void update(Result& result, const Args&... args) {
  auto* tracker = get_noise_tracker_if();
  double bound = NoisePolicy<Op>::compute(tracker, args...);
  tracker->update(result, bound);

  auto* variance_tracker = get_variance_tracker_if();
  double variance = VarianceNoisePolicy<Op>::compute(variance_tracker, args...);
  variance_tracker->update(result, variance);
}

}  // namespace evaluator

template <typename Op, typename... Feature>
class Evaluator {
 public:
  template <typename... Args>
  static auto exec(Args&&... args) {
    auto res = Op::exec_impl(std::forward<Args>(args)...);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      // Every exec_impl overload in tfhe/operation/{leveled,bootstrap}/ and
      // tfhe/gate/ takes its arguments by const&, so this forward never
      // actually moves from args; re-forwarding it here for the
      // noise/variance trackers reads it, never consumes it.
      // NOLINTNEXTLINE(bugprone-use-after-move)
      evaluator::update<Op>(res, std::forward<Args>(args)...);
    }
    return res;
  }
};

}  // namespace tfhe::operation

#endif