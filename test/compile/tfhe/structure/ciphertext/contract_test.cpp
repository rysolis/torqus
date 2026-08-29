#include "primitive/torus.hpp"

#include "tfhe/structure/ciphertext/concepts.hpp"

using Torus = ModTorus<16>;

static_assert(tlwe_ciphertext_concept<TLWE<Torus, 4>>);
static_assert(!tlwe_ciphertext_concept<TRLWE<Torus, 4>>);
static_assert(!tlwe_ciphertext_concept<int>);

static_assert(trlwe_ciphertext_concept<TRLWE<Torus, 4>>);
static_assert(!trlwe_ciphertext_concept<TLWE<Torus, 4>>);

static_assert(trgsw_ciphertext_concept<TRGSW<Torus, 4, 3>>);
static_assert(!trgsw_ciphertext_concept<TRLWE<Torus, 4>>);
