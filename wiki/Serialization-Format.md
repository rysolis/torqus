# Serialization Format

This page specifies the bytes that `tfhe/serialize.hpp` produces and
accepts. There are two layers:

- **Raw format** (`serialize::write` / `read` / `to_bytes` /
  `from_bytes`): the values only, with no header. Use it to exchange
  data with other libraries.
- **Envelope** (`serialize::write_enveloped` / `read_enveloped` /
  `to_bytes_enveloped` / `from_bytes_enveloped`): the raw format with a
  header in front. The header identifies the exact
  type. It is opt-in, for torqus-to-torqus storage and exchange.

Secret keys (`SecretHolder`) have no encoding in either layer.

## Conventions

- Every integer is little-endian, whatever the host's byte order.
- Nothing is aligned or padded, and the raw format has no length
  prefixes. Every length comes from the C++ type's template parameters.
- Every encoding has a fixed length that depends only on the type.
  `serialize::serialized_size<T>()` returns it.

## Raw format

### Scalars

| Type | Encoding | Bytes |
|---|---|---|
| `ModTorus<QBit, Word>` | `value()` as an unsigned `Word` | `sizeof(Word)` |
| `ModInt<P, Word>` | `value()` as an unsigned `Word` | `sizeof(Word)` |
| `dbl::Torus` | IEEE 754 binary64 bit pattern of `value()`, as a u64 | 8 |

`ModTorus` values are written as stored (already reduced to `QBit`
bits). On read, each raw value goes through the same constructor as
`T(raw)`, so any bits above `QBit` are masked off rather than rejected.
This keeps input written by other implementations readable.

### Containers

| Type | Encoding | Bytes |
|---|---|---|
| `Vector<T, Size>` | elements `[0]` … `[Size-1]` in order | `Size × size(T)` |
| `Poly<T, Size>` | coefficients `[0]` … `[Size-1]` in order | `Size × size(T)` |

### Ciphertexts and keys

| Type | Encoding (in order) | Bytes |
|---|---|---|
| `TLWE<Torus, n>` | `a` (`Vector<Torus, n>`), then `b` (`Torus`) | `(n + 1) × size(Torus)` |
| `TRLWE<Torus, N>` | `a` (`Poly<Torus, N>`), then `b` (`Poly<Torus, N>`) | `2N × size(Torus)` |
| `TRGSW<Torus, N, l>` | rows `[0]` … `[2l-1]`, each a `TRLWE<Torus, N>` | `2l × size(TRLWE)` |
| `BootstrapKey<Torus, N, l, n>` | `[0]` … `[n-1]`, each a `TRGSW<Torus, N, l>` | `n × size(TRGSW)` |
| `KeySwitchKey<Torus, n, t, m>` | `[i][j]` for `i` in `0..m-1` (outer), `j` in `0..t-1` (inner), each a `TLWE<Torus, n>` | `m × t × size(TLWE)` |
| `PublicKey<Ciphertext, PkSamples>` | `[0]` … `[PkSamples-1]`, each a `Ciphertext` | `PkSamples × size(Ciphertext)` |

Example: a `BootstrapKey<ModTorus<32, uint32_t>, 1024, 3, 630>` is
630 × 6 × 2 × 1024 × 4 = 30,965,760 bytes.

### What the raw format can and cannot detect

`from_bytes<T>` rejects input whose length isn't exactly
`serialized_size<T>()`. This catches a mismatch in `N`, `l`, `n`, `t`,
`m`, `PkSamples` or coefficient width. Two kinds of mix-up keep the same
length, so it can't catch them:

- different `QBit` with the same `Word` (e.g. `ModTorus<16, uint32_t>`
  and `ModTorus<32, uint32_t>`), or `ModTorus<64, uint64_t>` and
  `dbl::Torus`;
- dimensions whose products happen to match.

The stream `read<T>` doesn't check the length at all, since more data
may legitimately follow the value. Use the envelope when a mismatch has
to be caught.

## Envelope

| Field | Size | Value |
|---|---|---|
| magic | 4 bytes | the byte sequence `T` `Q` `S` `E` (`54 51 53 45`) |
| version | u32 | `1` |
| desc_len | u32 | number of u32 values in the descriptor |
| descriptor | u32 × desc_len | the type descriptor of `T` (below) |
| body | `serialized_size<T>()` | the raw format of the value, byte for byte |

Removing the first `12 + 4 × desc_len` bytes leaves the raw format.
`serialize::enveloped_size<T>()` returns the total length, and
`from_bytes_enveloped<T>` rejects input of any other length.

`read_enveloped<T>` checks magic, version, desc_len and the descriptor
in that order against what `T` expects. It throws on the first mismatch,
before reading the body.

### Type descriptor

A descriptor is a list of u32 values: a type tag, that type's own
dimensions, then the descriptor of its element type. The list is
recursive and ends at a scalar.

| Type | Descriptor |
|---|---|
| `ModTorus<QBit, Word>` | `1, QBit, sizeof(Word)` |
| `dbl::Torus` | `2` |
| `ModInt<P, Word>` | `3, P & 0xffffffff, P >> 32, sizeof(Word)` |
| `Vector<T, Size>` | `16, Size,` descriptor of `T` |
| `Poly<T, Size>` | `17, Size,` descriptor of `T` |
| `TLWE<Torus, n>` | `32, n,` descriptor of `Torus` |
| `TRLWE<Torus, N>` | `33, N,` descriptor of `Torus` |
| `TRGSW<Torus, N, l>` | `34, N, l,` descriptor of `Torus` |
| `BootstrapKey<Torus, N, l, n>` | `48, N, l, n,` descriptor of `Torus` |
| `KeySwitchKey<Torus, n, t, m>` | `49, n, t, m,` descriptor of `Torus` |
| `PublicKey<Ciphertext, PkSamples>` | `50, PkSamples,` descriptor of `Ciphertext` |

A ciphertext or key descriptor names only its own dimensions and its
`Torus`. Its nested parts (the TRLWE rows inside a TRGSW, for example)
are fully determined by those, so they aren't listed separately.

Example: `TLWE<ModTorus<32, uint32_t>, 630>` has the descriptor
`32, 630, 1, 32, 4`, so its envelope starts with these bytes (hex, little-endian):

```
54 51 53 45  01 00 00 00  05 00 00 00
20 00 00 00  76 02 00 00  01 00 00 00  20 00 00 00  04 00 00 00
<2524 bytes of raw TLWE>
```

## Choosing the format in one place

`serialize::Raw` and `serialize::Enveloped`
([`format.hpp`](https://github.com/rysolis/torqus/blob/main/include/tfhe/serialize/format.hpp))
are the two layers as types. Each has `write`, `read<T>`, `to_bytes`
and `from_bytes<T>`. Name the format once and call it everywhere:

```cpp
using Format = serialize::Enveloped;  // or serialize::Raw
Format::write(file, bootstrap_key);
auto bk = Format::read<decltype(bootstrap_key)>(file);
```

Changing that one alias switches every call site.

## Transport

`tfhe/transport.hpp` picks the format for a protocol hand-off by
template parameter:

| Transport | Wire bytes |
|---|---|
| `DirectTransport` | none (the C++ value is passed as is) |
| `SerializedTransport` | raw format |
| `EnvelopedTransport` | envelope |

`SerializedTransport` and `EnvelopedTransport` are
`BytesTransport<serialize::Raw>` and `BytesTransport<serialize::Enveloped>`.

## Errors

| Exception | When |
|---|---|
| `serialize::format_error` | `from_bytes` or `from_bytes_enveloped` length mismatch; envelope with a bad magic, an unsupported version or a descriptor mismatch |
| `std::out_of_range` | a stream that ends before the value is complete |

## Changing the format

- **Raw format.** Any change breaks compatibility with data and
  implementations outside torqus. A new type gets a new encoding;
  existing encodings don't change.
- **Tags.** The values in the descriptor table are fixed. A new
  serializable type gets an unused tag (`TypeTag` in
  [`descriptor.hpp`](https://github.com/rysolis/torqus/blob/main/include/tfhe/serialize/descriptor.hpp)),
  and a tag is never renumbered or reused. The `static_assert`s in
  [`serialize_test.cpp`](https://github.com/rysolis/torqus/blob/main/test/runtime/tfhe/serialize/serialize_test.cpp)
  pin a sample of descriptors.
- **Envelope version.** Bump it only when the envelope header itself
  changes. A new tag doesn't need a bump: an older reader already
  rejects a descriptor it doesn't expect.
