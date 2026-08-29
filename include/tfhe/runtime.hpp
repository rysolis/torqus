// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_RUNTIME_HPP
#define TFHE_RUNTIME_HPP

#include <memory>

#include "primitive/concept/torus.hpp"
#include "primitive/uint.hpp"

#include "tfhe/cryptor.hpp"
#include "tfhe/cryptor/traits.hpp"
#include "tfhe/feature.hpp"
#include "tfhe/params.hpp"
#include "tfhe/structure/key/bootstrap_key.hpp"
#include "tfhe/structure/key/key_switch_key.hpp"
#include "tfhe/structure/key/public_key.hpp"
#include "tfhe/utility/analysis/tracker_if.hpp"
#include "tfhe/utility/secret_holder.hpp"

template <typename Params>
struct secret_size;

template <tlwe_concept Params>
struct secret_size<Params> {
  static constexpr uint32_t value = Params::n;
};

template <typename Params>
  requires trlwe_concept<Params>
struct secret_size<Params> {
  static constexpr uint32_t value = Params::N;
};

// Runtime owns a Cryptor<Params> internally -- Cryptor is an implementation
// detail of "how to encrypt/decrypt under these Params", not something a
// caller of Runtime should need to know about or spell out.
template <typename Params, typename... Feature>
class Runtime {
 private:
  using cryptor_type = Cryptor<Params>;

 public:
  using params_type = Params;
  static constexpr uint32_t secret_length = secret_size<params_type>::value;

  Runtime() = default;

  template <typename Engine>
    requires std::uniform_random_bit_generator<Engine>
  Runtime(Engine& eng) {
    holder_.emplace(eng);
    cryptor_.emplace(holder_->shared_get(), eng);
  }

  template <typename Engine, typename OtherParams, typename... OtherFeature>
  Runtime(const Runtime<OtherParams, OtherFeature...>& other, Engine& eng)
      : holder_(other.holder()) {
    cryptor_.emplace(holder_->shared_get(), eng);
  }

  // encrypt()/decrypt() here do no more than call through to Cryptor (plus
  // optional noise tracking), so their result types are exactly Cryptor's
  // -- named via the same generic ciphertext_t/plaintext_t used elsewhere
  // (see cryptor/traits.hpp), rather than duplicating the type as a
  // Runtime-local alias.
  //
  // Spelled out explicitly here (rather than auto, as
  // Cryptor's own encrypt()/decrypt() use) because this is the public boundary:
  // callers should be able to name the result type off the signature alone, per
  // the ciphertext_t/plaintext_t comment above, without needing to trace
  // through to Cryptor to find out what auto resolves to.
  template <typename Plaintext>
  ciphertext_t<cryptor_type, Plaintext> encrypt(const Plaintext& pt) {
    ciphertext_t<cryptor_type, Plaintext> res = cryptor().encrypt(pt);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      get_noise_tracker_if()->update(res, fresh_noise_bound<Params>());
      // The real variance behind that same worst-case bound (alpha^2, not
      // the 6-sigma-tail-cutoff bound above it) -- see
      // tfhe/utility/analysis/variance_noise.hpp's VarianceNoisePolicy, the
      // one intended reader.
      get_variance_tracker_if()->update(
          res, alpha_of<Params>::value * alpha_of<Params>::value);
    }
    return res;
  }

  template <typename Ciphertext>
  plaintext_t<cryptor_type, Ciphertext> decrypt(const Ciphertext& ct) {
    return cryptor().decrypt(ct);
  }

  template <typename Lwe, typename Rlwe, typename Decomp>
    requires tlwe_concept<Lwe> && decompose_concept<Decomp>
  BootstrapKey<typename Rlwe::torus_type, Rlwe::N, Decomp::l, Lwe::n>
  generate_bootstrap_key(const UInt::raw_value_type* secret) {
    auto bk = bootstrap_key::generate<Lwe, Rlwe, Decomp>(cryptor(), secret);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      get_key_noise_tracker_if()->update(bk, fresh_noise_bound<Params>());
      get_key_variance_tracker_if()->update(
          bk, alpha_of<Params>::value * alpha_of<Params>::value);
    }
    return bk;
  }

  template <typename SrcLwe, typename DstLwe, typename Kst>
    requires tlwe_concept<SrcLwe> && tlwe_concept<DstLwe> && kst_concept<Kst>
  KeySwitchKey<typename DstLwe::torus_type, DstLwe::n, Kst::t, SrcLwe::n>
  generate_key_switch_key(const UInt::raw_value_type* secret) {
    auto ksk = key_switch_key::generate<SrcLwe, DstLwe, Kst>(cryptor(), secret);
    if constexpr ((std::same_as<Tracking, Feature> || ...)) {
      // See the matching comment in generate_bootstrap_key above -- same
      // reasoning, using this Runtime's own (destination-side) Params.
      get_key_noise_tracker_if()->update(ksk, fresh_noise_bound<Params>());
      get_key_variance_tracker_if()->update(
          ksk, alpha_of<Params>::value * alpha_of<Params>::value);
    }
    return ksk;
  }

  // Generates a public key of PkSamples TLWE(0) samples under this
  // Runtime's own secret -- see public_key.hpp for how it lets a caller
  // who never sees that secret encrypt under it anyway.
  template <uint32_t PkSamples>
    requires tlwe_concept<Params>
  PublicKey<typename Params::torus_type, secret_length, PkSamples>
  generate_public_key() {
    using Torus = typename Params::torus_type;
    PublicKey<Torus, secret_length, PkSamples> pk;
    for (uint32_t i = 0; i < PkSamples; ++i) {
      pk[i] = encrypt(Torus(0u));
    }
    return pk;
  }

  // holder_ is only ever left unset by Runtime() = default, used solely as
  // a placeholder member (see the test fixtures under test/runtime/) that
  // gets overwritten by assignment from a properly constructed Runtime
  // before this is ever called.
  // NOLINTNEXTLINE(bugprone-exception-escape)
  const SecretHolder<secret_length>& holder() const noexcept {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return holder_.value();
  }

 private:
  // cryptor_ carries the same never-unset-when-called invariant as holder_
  // above.
  cryptor_type& cryptor() {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return cryptor_.value();
  }

  std::optional<SecretHolder<secret_length>> holder_;
  std::optional<cryptor_type> cryptor_;
};

#endif