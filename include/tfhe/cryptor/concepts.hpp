// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef TFHE_CRYPTOR_CONCEPTS_HPP
#define TFHE_CRYPTOR_CONCEPTS_HPP

// Whether some encryptor type `Enc` (e.g. Cryptor<Params> or
// Runtime<Params>) can encrypt/decrypt a given Plaintext/Ciphertext at
// all -- guards ciphertext_t/plaintext_t below so that an unsupported
// combination fails as "constraint not satisfied" rather than as a raw
// decltype substitution error.
template <typename Enc, typename Plaintext>
concept encryptable_concept =
    requires(Enc& enc, const Plaintext& pt) { enc.encrypt(pt); };

template <typename Enc, typename Ciphertext>
concept decryptable_concept =
    requires(Enc& enc, const Ciphertext& ct) { enc.decrypt(ct); };

#endif