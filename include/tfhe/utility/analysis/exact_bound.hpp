// Copyright 2026, Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_UTILITY_EXACT_BOUND_HPP
#define TFHE_UTILITY_EXACT_BOUND_HPP

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>
#include <cmath>
#include <limits>

// All quantities NoisePolicy<Op> combines (eps terms, key-noise bounds,
// fresh-noise bounds, ...) are exact dyadic rationals -- real Gaussian
// samples are cut off at a fixed tail (see fresh_noise_bound/kNoiseTailSigma
// in noise.hpp) and folded in as that deterministic multiple of alpha, never
// as the sample itself -- so summing them as `double` risks silently losing
// small terms once many of them (large n, N, l, t, ...) get added together
// with wildly different magnitudes. ExactBound performs that accumulation
// with unbounded-precision exact rational arithmetic instead, so the only
// place any rounding can happen is the single, explicit, round-up
// conversion back to `double` at the end (see `to_round_up`).
using ExactBound = boost::rational<boost::multiprecision::cpp_int>;

// Every finite double is itself an exact dyadic rational (mantissa *
// 2^exponent), so this reconstruction never loses precision.
inline ExactBound to_exact(double d) {
  if (d == 0.0) {
    return ExactBound(0);
  }
  int exp = 0;
  const double mantissa = std::frexp(d, &exp);  // d = mantissa * 2^exp
  constexpr int kMantissaBits = 53;  // double has 53 significand bits
  const auto numerator = boost::multiprecision::cpp_int(static_cast<long long>(
      mantissa * static_cast<double>(1LL << kMantissaBits)));
  const int shift = exp - kMantissaBits;
  if (shift >= 0) {
    return ExactBound(numerator << shift, 1);
  }
  return ExactBound(numerator, boost::multiprecision::cpp_int(1) << (-shift));
}

// Converts an exact rational bound to the smallest double known to be >= it
// with high confidence, so a tracked noise bound can never silently
// understate the true worst-case error because of this final rounding step.
// (A single `nextafter` nudge covers the one division's rounding direction;
// it is not a formally verified directed-rounding division, but the
// accumulation itself -- the actual risk this type exists to remove -- is
// exact.)
inline double to_round_up(const ExactBound& r) {
  if (r == 0) {
    return 0.0;
  }
  const double approx =
      r.numerator().convert_to<double>() / r.denominator().convert_to<double>();
  return std::nextafter(approx, std::numeric_limits<double>::infinity());
}

#endif  // TFHE_UTILITY_EXACT_BOUND_HPP
