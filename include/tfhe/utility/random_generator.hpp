// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_RANDOM_GENERATOR_HPP
#define TFHE_RANDOM_GENERATOR_HPP

#include <concepts>
#include <random>

template <typename Engine>
struct RandomGenerator : public Engine {
 public:
  RandomGenerator() : Engine(std::random_device{}()) {}
  explicit RandomGenerator(typename Engine::result_type seed) : Engine(seed) {}
};

// A std::uniform_random_bit_generator suitable for real key/noise
// generation -- unlike RandomGenerator<std::mt19937> above, which every
// test in this repository uses purely for its fixed, reproducible seed.
// std::mt19937 is not cryptographically secure: its full internal state is
// recoverable from ~624 successive 32-bit outputs, so anything it produces
// -- secret bits, noise samples -- becomes predictable once that many
// outputs leak. Pass a SecureRandomEngine directly to Runtime/PublicRuntime
// instead for anything beyond a test, e.g.
// `SecureRandomEngine rng; Runtime<Params, SecureRandomEngine, Tracking>
// runtime(rng);` -- no RandomGenerator<> wrapper needed, since this has no
// seed to fix.
//
// std::random_device's own output quality is implementation-defined by the
// C++ standard, but is backed by the OS CSPRNG on every major platform
// (Linux getrandom(2)/urandom, macOS/BSD arc4random, Windows
// BCryptGenRandom) -- deliberately not a hand-rolled DRBG/stream cipher,
// per "don't roll your own crypto."
using SecureRandomEngine = std::random_device;

#endif