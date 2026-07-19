// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_EVALUATOR_HPP
#define TFHE_EVALUATOR_HPP

#include "tfhe/feature.hpp"
#include "tfhe/utility/analysis/noise.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"

namespace evaluator {

template <typename Op, typename Result, typename... Args>
void update(Result& result, const Args&... args) {
  auto* tracker = get_noise_tracker_if();

  double bound = NoisePolicy<Op>::compute(tracker, args...);

#ifndef NDEBUG
  if (bound >= 0.25) {
    std::cerr << "error_bound = " << bound << '\n';
  }
  assert(bound < 0.25);
#endif

  tracker->update(result, bound);
}

}  // namespace evaluator

template <typename Op, typename... Feature>
class Evaluator {
 public:
  template <typename... Args>
  static auto exec(Args&&... args) {
    auto res = Op::exec_impl(std::forward<Args>(args)...);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      evaluator::update<Op>(res, std::forward<Args>(args)...);
    }
    return res;
  }
};

#endif