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

#endif