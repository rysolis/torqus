// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CIRCUIT_XOR_HPP
#define TFHE_CIRCUIT_XOR_HPP

#include "tfhe/bit/cipher.hpp"
#include "tfhe/gate/hom_xor.hpp"
#include "tfhe/operation/bootstrap/gate_bootstrap.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"

// Wraps tfhe::gate::HomXor -- see tfhe/circuit/and.hpp's own doc comment,
// which applies here unchanged (same shape, same reasoning).
namespace tfhe::circuit {

template <typename Lwe, typename Rlwe, typename Decomp,
          template <typename, typename, typename> class Backend =
              tfhe::bootstrap::GateBootstrap>
class Xor {
 public:
  using Torus = typename Lwe::torus_type;
  static constexpr uint32_t n = Lwe::n;

  using rTorus = typename Rlwe::torus_type;
  static constexpr uint32_t N = Rlwe::N;

  static constexpr uint32_t l = Decomp::l;

  Xor() = default;
  explicit Xor(const BootstrapKey<rTorus, N, l, n>& bk) : bk_(&bk) {}

  Cipher<Lwe, Rlwe> exec(const Cipher<Lwe, Rlwe>& lhs,
                         const Cipher<Lwe, Rlwe>& rhs) const {
    return Cipher<Lwe, Rlwe>(
        tfhe::gate::HomXor<Lwe, Rlwe, Decomp, Backend>::exec_impl(
            lhs.ready(), rhs.ready(), *bk_));
  }

 private:
  const BootstrapKey<rTorus, N, l, n>* bk_;
};

}  // namespace tfhe::circuit

#endif  // TFHE_CIRCUIT_XOR_HPP
