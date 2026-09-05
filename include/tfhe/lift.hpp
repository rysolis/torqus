// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_LIFT_HPP
#define TFHE_LIFT_HPP

#include <cstdint>

#include "tfhe/bit.hpp"
#include "tfhe/dial.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"

// Lift<Resolution, Lwe, Rlwe, Feature...> holds a Runtime<Lwe, Feature...>
// and exposes only encrypt() -- a plaintext-to-ciphertext boundary a
// caller can be handed without also being handed decrypt(). Combines
// Dial<Resolution, Torus> (index -> Torus) with Runtime::encrypt()
// (Torus -> Bit) into one step. Feature is the same optional feature
// pack (e.g. Tracking) Runtime itself takes.
template <uint32_t Resolution, typename Lwe, typename Rlwe, typename... Feature>
class Lift {
 public:
  using Torus = typename Lwe::torus_type;
  using Plain = Dial<Resolution, Torus>;

  explicit Lift(Runtime<Lwe, Feature...>& runtime) : runtime_(runtime) {}

  Bit<Lwe, Rlwe> encrypt(uint32_t index) const {
    return runtime_.encrypt(Plain(index).value());
  }

 private:
  Runtime<Lwe, Feature...>& runtime_;
};

// Drop<Resolution, Lwe, Rlwe, Decomp, Feature...> holds a
// Runtime<ParamsPack<Rlwe, Decomp>, Feature...> and exposes only
// decrypt() -- the ciphertext-to-plaintext counterpart to Lift. Combines
// Runtime::decrypt() (Bit -> rTorus) with Dial<Resolution, rTorus>
// (rTorus -> index) into one step.
template <uint32_t Resolution, typename Lwe, typename Rlwe, typename Decomp,
          typename... Feature>
class Drop {
 public:
  using rTorus = typename Rlwe::torus_type;
  using RPlain = Dial<Resolution, rTorus>;

  explicit Drop(Runtime<ParamsPack<Rlwe, Decomp>, Feature...>& runtime)
      : runtime_(runtime) {}

  uint32_t decrypt(const Bit<Lwe, Rlwe>& bit) const {
    return RPlain(runtime_.decrypt(bit.pending())).index();
  }

 private:
  Runtime<ParamsPack<Rlwe, Decomp>, Feature...>& runtime_;
};

#endif  // TFHE_LIFT_HPP
