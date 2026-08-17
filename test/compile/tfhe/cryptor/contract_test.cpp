#include "tfhe/cryptor/cryptor.hpp"

#include "algebra/poly.hpp"
#include "primitive/torus.hpp"
#include "primitive/uint.hpp"
#include "tfhe/cryptor/traits.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"

// TLWE: Cryptor<Params> for a tlwe_concept Params encrypts a bare Torus
// value into a TLWE ciphertext of that Params' own dimension.
using TlweParams = lwe_params<tlwe_core_params<ModTorus<16>, 4>>;
static_assert(std::same_as<ciphertext_t<Cryptor<TlweParams>, ModTorus<16>>,
                           TLWE<ModTorus<16>, 4>>);
static_assert(
    std::same_as<plaintext_t<Cryptor<TlweParams>, TLWE<ModTorus<16>, 4>>,
                 ModTorus<16>>);

// TRLWE: Cryptor<Params> for a trlwe_concept Params encrypts a Poly of
// Torus values into a TRLWE ciphertext of that Params' own ring dimension.
using TrlweParams = rlwe_params<trlwe_core_params<ModTorus<16>, 4>>;
static_assert(
    std::same_as<ciphertext_t<Cryptor<TrlweParams>, Poly<ModTorus<16>, 4>>,
                 TRLWE<ModTorus<16>, 4>>);
static_assert(
    std::same_as<plaintext_t<Cryptor<TrlweParams>, TRLWE<ModTorus<16>, 4>>,
                 Poly<ModTorus<16>, 4>>);

// TRGSW: the same trlwe_concept Params, when also decompose_concept
// (bundled via ParamsPack), instead encrypts a Poly<UInt> message into a
// TRGSW ciphertext -- the same Cryptor<Params> dispatches to a different
// ciphertext_t depending on the Plaintext handed to encrypt().
using TrgswParams = ParamsPack<TrlweParams, dcp_params<4, 3>>;
static_assert(
    std::same_as<ciphertext_t<Cryptor<TrgswParams>, Poly<UInt, 4>>,
                 TRGSW<ModTorus<16>, 4, 3>>);

// encryptable_concept/decryptable_concept correctly reject unsupported
// combinations instead of forcing a hard error.
static_assert(!encryptable_concept<Cryptor<TlweParams>, Poly<ModTorus<16>, 4>>);
static_assert(!decryptable_concept<Cryptor<TlweParams>, TRLWE<ModTorus<16>, 4>>);

// Runtime<Params> supports the same generic ciphertext_t/plaintext_t,
// since Runtime::encrypt/decrypt add only optional noise tracking on top
// of Cryptor::encrypt/decrypt.
static_assert(
    std::same_as<ciphertext_t<Runtime<TlweParams>, ModTorus<16>>,
                 TLWE<ModTorus<16>, 4>>);
static_assert(std::same_as<
              plaintext_t<Runtime<TlweParams>, TLWE<ModTorus<16>, 4>>,
              ModTorus<16>>);
