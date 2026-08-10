// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_RUNTIME_HPP
#define TFHE_RUNTIME_HPP

#include <memory>

#include "primitive/concept/torus.hpp"
#include "primitive/uint.hpp"

#include "tfhe/feature.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"
#include "tfhe/utility/secret_holder.hpp"

template <typename Cryptor, typename... Feature>
class Runtime {
 public:
  using params_type = Cryptor::params_type;
  using iterator = SecretHolder<params_type>::raw_value_type*;
  using const_iterator = const SecretHolder<params_type>::raw_value_type*;

  Runtime() = default;

  template <typename Engine>
  Runtime(Engine& eng) {
    holder_.emplace(eng);
    cryptor_.emplace(holder_->secret_ptr(), eng);
  }

  template <std::forward_iterator It, typename Engine>
  Runtime(It first, It last, Engine& eng) {
    holder_ = SecretHolder<params_type>(first, last);
    cryptor_.emplace(holder_->secret_ptr(), eng);
  }

  template <typename Plaintext>
  auto encrypt(const Plaintext& pt) {
    auto res = cryptor_->encrypt(pt);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      auto* tracker = get_noise_tracker_if();
      tracker->update(res, 0.);
    }
    return res;
  }

  template <typename Ciphertext>
  auto decrypt(const Ciphertext& ct) {
    return cryptor_->decrypt(ct);
  }

  template <typename Lwe, typename Rlwe, typename Dcp>
    requires tlwe_concept<Lwe> && decompose_concept<Dcp>
  BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Dcp::l, Lwe::n>
  generate_bootstrap_key(const UInt::raw_value_type* s) {
    auto bk = bootstrap_key::generate<Lwe, Rlwe, Dcp>(*cryptor_, s);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      auto* tracker = get_key_noise_tracker_if();
      tracker->update(bk, 0.);
    }
    return bk;
  }

  template <typename SrcLwe, typename DstLwe, typename Kst>
    requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
  KeySwitchKey<typename DstLwe::torus_type, DstLwe::n, Kst::t, SrcLwe::n>
  generate_key_switch_key(const UInt::raw_value_type* s) {
    auto ksk = keyswitch_key::generate<SrcLwe, DstLwe, Kst>(*cryptor_, s);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      auto* tracker = get_key_noise_tracker_if();
      tracker->update(ksk, 0.);
    }
    return ksk;
  }

  const SecretHolder<params_type>::raw_value_type* secret() const noexcept {
    return holder_->secret();
  }

  iterator begin() noexcept { return holder_->begin(); }
  iterator end() noexcept { return holder_->end(); }

  const_iterator begin() const noexcept { return holder_.begin(); }
  const_iterator end() const noexcept { return holder_->end(); }

 private:
  std::optional<SecretHolder<params_type>> holder_;
  std::optional<Cryptor> cryptor_;
};

#endif