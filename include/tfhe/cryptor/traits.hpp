#ifndef TFHE_CRYPTOR_TRAITS_HPP
#define TFHE_CRYPTOR_TRAITS_HPP

#include <utility>

#include "tfhe/cryptor/concepts.hpp"

// Named aliases for encrypt()/decrypt()'s otherwise-opaque `auto` return
// types, so callers can name "the ciphertext type this Enc produces for a
// given Plaintext" (and vice versa) without tracing the dispatch above.
// Generic over any encryptor-shaped type, not just Cryptor itself --
// Runtime forwards to these too (see runtime.hpp).
template <typename Enc, typename Plaintext>
  requires encryptable_concept<Enc, Plaintext>
using ciphertext_t =
    decltype(std::declval<Enc&>().encrypt(std::declval<const Plaintext&>()));

template <typename Enc, typename Ciphertext>
  requires decryptable_concept<Enc, Ciphertext>
using plaintext_t =
    decltype(std::declval<Enc&>().decrypt(std::declval<const Ciphertext&>()));

#endif