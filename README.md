<div align="center">

<h1>torqus</h1>

[![CI](https://github.com/rysolis/torqus/actions/workflows/ci.yml/badge.svg)](https://github.com/rysolis/torqus/actions/workflows/ci.yml)
[![Lint](https://github.com/rysolis/torqus/actions/workflows/lint.yml/badge.svg)](https://github.com/rysolis/torqus/actions/workflows/lint.yml)
[![codecov](https://codecov.io/gh/rysolis/torqus/branch/main/graph/badge.svg)](https://codecov.io/gh/rysolis/torqus)
[![License: Apache 2.0](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)
[![Release](https://img.shields.io/github/v/release/rysolis/torqus)](https://github.com/rysolis/torqus/releases)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](CMakeLists.txt)

**torqus** (pronounced "torks" -- torque + Torus) is a C++20,
header-only TFHE library for mathematicians and developers, exposing
TLWE/TRLWE/TRGSW ciphertexts, leveled arithmetic, and gate bootstrapping
(AND, AND-NOT, ...) with opt-in runtime noise tracking.

</div>

**Concept:** a cryptography library a mathematician can actually read
and a developer can actually ship -- types and primitives named close
to the TFHE paper's own notation (`TLWE`/`TRLWE`/`TRGSW`, `n`/`N`,
`alpha`, ...) rather than hidden behind opaque byte buffers, so each
piece of code traces directly back to the paper construct it
implements -- engineered to the correctness and rigor a production
deployment needs, not just a research prototype.

Every noise/dimension parameter (`n`, `N`, `Bg`, `l`, `alpha`, ...) is a
compile-time template argument you choose yourself -- torqus ships no
fixed parameter set; pick whatever meets your own security, performance,
and post-bootstrap noise margin target (see [Bootstrap Noise
Bounds](#bootstrap-noise-bounds) -- `Bg`/`l` trade directly against how
much noise a `GateBootstrap` leaves behind, and `KeySwitch`'s own
decomposition params (`K`/`t`, `kst_params`) trade the same way against
the noise a `KeySwitch` adds).

## Table of Contents

- [Security Status](#security-status)
- [Supported Operations](#supported-operations)
- [Bootstrap Noise Bounds](#bootstrap-noise-bounds)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [Conan](#conan)
  - [vcpkg](#vcpkg)
  - [CMake `add_subdirectory()` / `FetchContent`](#cmake-add_subdirectory--fetchcontent)
  - [Example: a homomorphic AND gate](#example-a-homomorphic-and-gate)
- [Developing torqus](DEVELOPING.md)
- [References](#references)

***

## Security Status

This is a from-scratch TFHE implementation and has **not** undergone an
independent third-party security audit. The noise tracker
(`tfhe/utility/analysis/`) enforces the noise bounds it's told to expect,
which catches parameter-mismatch bugs, but that is not a substitute for
a cryptographic review of the scheme's implementation itself. Treat it
accordingly before relying on it for anything security-critical.

***

## Supported Operations

Everything below is reachable through the umbrella headers at `tfhe/`'s
root (see [Project Structure](DEVELOPING.md#project-structure)); each row links to
the umbrella header that pulls in its subdirectory.

| Category | Types / Operations | Header |
| --- | --- | --- |
| Ciphertexts | `TLWE`, `TRLWE`, `TRGSW` | [`tfhe/ciphertext.hpp`](include/tfhe/ciphertext.hpp) |
| Leveled arithmetic | `Add`, `Sub`, `KeySwitch`, `SampleExtract` | [`tfhe/operation.hpp`](include/tfhe/operation.hpp) |
| Bootstrap primitives | `BlindRotate`, `ExternalProduct`, `CMux`, `GateBootstrap` | [`tfhe/operation.hpp`](include/tfhe/operation.hpp) |
| Gates | `HomAnd`, `HomAndNot`, `HomOr`, `HomXor` | [`tfhe/gate.hpp`](include/tfhe/gate.hpp) |
| Plaintext codec | `Dial` (names a Torus value by one of `Resolution` evenly-spaced slots) | [`tfhe/dial.hpp`](include/tfhe/dial.hpp) |
| Ciphertext state | `Bit` (hides whether a ciphertext is Lwe- or Rlwe-shaped), `Circuit`/`Relay` (gate calls and materializing as method calls, not raw key arguments) | [`tfhe/bit.hpp`](include/tfhe/bit.hpp), [`tfhe/scope.hpp`](include/tfhe/scope.hpp) |
| Plaintext/ciphertext boundary | `Lift` (encrypt-only: plaintext -> `Bit`), `Drop` (decrypt-only: `Bit` -> plaintext) -- each holds a `Runtime` privately, exposing only its one direction | [`tfhe/lift.hpp`](include/tfhe/lift.hpp) |
| Circuits | `BinaryExpansion` (gate-level binary expansion, built on `Bit`/`Circuit`/`Relay`) | [`tfhe/circuit.hpp`](include/tfhe/circuit.hpp) |
| Serialization | wire (de)serialization for the ciphertext/key types above | [`tfhe/serialize.hpp`](include/tfhe/serialize.hpp) |

Gates take Lwe-shaped ciphertexts in and return a fresh Rlwe-domain
ciphertext out (see the `HomAnd` example below); chaining several
together, as `BinaryExpansion` does, needs a `KeySwitch` back down to
Lwe between calls -- `Bit`/`Circuit`/`Relay` wrap that dance behind
`Circuit::And`/`Or`/`AndNot`/`Xor` and an explicit `Relay::materialize()`
instead of every call site juggling raw keys and `KeySwitch` by hand.

***

## Bootstrap Noise Bounds

Bootstrapping discards the input ciphertext's own noise entirely and
replaces it with fresh noise derived only from the bootstrap key
(RLWE-side alpha) and the gadget decomposition (`Bg`, `l`), regardless
of how much noise the ciphertext going in carried:

| Context | n | N | Bg | l | alpha (RLWE) | predicted σ | 99% threshold (sub-Gaussian) | 99% threshold (Gaussian est.) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 128-bit security | 630 | 1024 | 16 | 7 | 2⁻²⁵ | 7.181×10⁻⁴ | 2.338×10⁻³ | 1.850×10⁻³ |

Both thresholds are well under the 0.25 decryption margin `GateBootstrap`
must clear to decode its own output correctly at all -- a hard
requirement, not a target. How far under it you actually need to be is
a separate question with no universal answer: it's whatever precision
your own use case demands, so choose your own `n`/`N`/`Bg`/`l`/`alpha`
accordingly. Every gate here ends in its own `GateBootstrap` call (see
[Supported Operations](#supported-operations) -- `BinaryExpansion`
key-switches back to Lwe between gates rather than chaining leveled ops
across a single bootstrap's output), so this requirement applies
independently to each per-gate bootstrap.

For the statistical methodology behind these numbers -- why the proven
sub-Gaussian bound, not the Gaussian estimate, is what's actually
checked; how [`gate_bootstrap_test.cpp`](test/runtime/tfhe/operation/bootstrap/gate_bootstrap_test.cpp)
computes and checks them; and planned future work -- see the
[Bootstrap Noise Bounds wiki page](https://github.com/rysolis/torqus/wiki/Bootstrap-Noise-Bounds).

***

## Usage

### Quick Start

torqus is header-only, so trying it needs no package manager and no
build step for the library itself -- just get `include/` onto your
compiler's include path and start including headers:

```bash
git clone https://github.com/rysolis/torqus.git
c++ -std=c++20 -Itorqus/include your_program.cpp -o your_program
```

Using CMake instead: see [CMake `add_subdirectory()` /
`FetchContent`](#cmake-add_subdirectory--fetchcontent) below for the
`target_link_libraries(your_target PRIVATE torqus::torqus)` route.

Core usage (TLWE/TRLWE/TRGSW, leveled ops, gate bootstrapping) needs
nothing beyond a C++20 compiler; Boost (`libboost-dev`) is only pulled in
if you use the noise-tracking `Tracking` feature
(`tfhe/utility/analysis/`). See [Example: a homomorphic AND
gate](#example-a-homomorphic-and-gate) below for a full working program,
and [Supported Operations](#supported-operations) for what's available
to build with.

torqus imposes no optimization or debug flags of its own -- the CMake
target it installs (`torqus::torqus`) is a plain `INTERFACE` library that
only requires `cxx_std_20` and, optionally, the two feature macros above;
it carries no `-O*`/`-g` of any kind. Set `-O2`/`-O3`, `-march=native`,
`CMAKE_BUILD_TYPE`, etc. the same way you would for any other dependency
in your own project (e.g. `target_compile_options(your_target PRIVATE
-O3)`, or just your build type) -- torqus never overrides them.

By default, `encrypt()` samples real Gaussian noise for any param set
that opts into it via `noise_params<AlphaBits>` (`tfhe/params.hpp`). To
force every ciphertext back to exact/noiseless regardless of
`noise_params` -- e.g. for debugging against the old noiseless behavior --
define `TFHE_DISABLE_NOISE` before including any torqus header (in this
repository's own CMake build, the equivalent is `-DTFHE_ENABLE_NOISE=OFF`,
see [Developing torqus](DEVELOPING.md)).

`UInt` and `ModTorus<QBit>` (`primitive/uint.hpp`, `primitive/torus.hpp`)
store their raw value in a `uint32_t` by default. To make that a
`uint64_t` instead -- e.g. to run `ModTorus<64>`, or any `QBit` above
32 -- define `TORQUS_TORUS_BITS=64` before including any torqus header:

```bash
c++ -std=c++20 -DTORQUS_TORUS_BITS=64 -Itorqus/include your_program.cpp -o your_program
```

or, in this repository's own CMake build (or a consumer's, via
`torqus::torqus`), `-DTORQUS_TORUS_BITS=64` at configure time does the
same. This only changes the *default* word width: an individual
`ModTorus<QBit, Word>` (or `ModInt<P, Word>`) can always name a specific
`Word` (e.g. `ModTorus<48, uint64_t>`) regardless of this setting, and
existing code naming just `ModTorus<QBit>` picks up whatever the setting
resolves to. NEON acceleration (see [Performance &
Concurrency](DEVELOPING.md#performance--concurrency)) only covers 32-bit elements
today, so a 64-bit `Word` always takes the portable scalar add/sub path.

Beyond the plain-clone Quick Start above, vendoring `include/` as a git
submodule works the same way. For dependency-managed integration
instead, the C++ core is also packaged for two package managers under
the name `torqus`:

### Conan

[`conanfile.py`](conanfile.py) packages `include/` behind `find_package(torqus)`
(`torqus::torqus`), backed by this repository's own `install()` rules
(`CMakeLists.txt`) plus [`cmake/torqusConfig.cmake.in`](cmake/torqusConfig.cmake.in).
`mimalloc`/SIMD/noise are exposed as Conan options (`use_mimalloc`,
`enable_simd`, `enable_noise`), mirroring the `TORQUS_USE_MIMALLOC` /
`TORQUS_ENABLE_SIMD` / `TFHE_ENABLE_NOISE` CMake options below:

```bash
conan create . --build=missing
```

This builds and exports the package to your local Conan cache; publishing to
ConanCenter is not set up yet, but is planned.

### vcpkg

[`vcpkg.json`](vcpkg.json) is the package manifest (name `torqus`, a
`mimalloc` feature on by default). Building it as a vcpkg **port** needs a
`portfile.cmake` as well, which lives in a vcpkg registry rather than in
this repository -- see the project's vcpkg registry setup for that piece.
Publishing that registry (and thus `vcpkg install torqus` support) isn't
done yet, but is planned.

### CMake `add_subdirectory()` / `FetchContent`

No package manager and no install step needed: `torqus-libs`
(`torqus::torqus`) is a plain target defined in this repository's own
`CMakeLists.txt`, so pulling the repository into your own CMake project
gets you that target directly --

```cmake
include(FetchContent)
FetchContent_Declare(torqus
  GIT_REPOSITORY https://github.com/rysolis/torqus.git
  # "main" is a branch, not pinned -- for reproducible builds, use a
  # release tag or commit hash here instead.
  GIT_TAG main)
FetchContent_MakeAvailable(torqus)

target_link_libraries(your_target PRIVATE torqus::torqus)
```

Or, vendoring `torqus` as a git submodule instead of fetching it:

```cmake
add_subdirectory(path/to/torqus)

target_link_libraries(your_target PRIVATE torqus::torqus)
```

`PROJECT_IS_TOP_LEVEL` in this repository's own `CMakeLists.txt` keeps
its own test suite (GoogleTest, Boost, mimalloc, ...) from being pulled
into your build even if your project also has `BUILD_TESTING` on.

### Example: a homomorphic AND gate

Encrypt two bits under a `Runtime` and combine them with a `Circuit`:
[`Bit`](include/tfhe/bit.hpp) hides whether a ciphertext is Lwe- or
Rlwe-shaped, [`Circuit`](include/tfhe/scope.hpp) turns a gate call into a
method call that doesn't need `bk` (or `<Decomp>`) spelled out by hand --
its own signature is the guarantee that combining ciphertexts never
touches the secret -- and [`Lift`/`Drop`](include/tfhe/lift.hpp) each
hold a `Runtime` privately so whoever holds one can only encrypt or only
decrypt, not both. Everything below comes in through the umbrella
headers at `tfhe/`'s root (see [Project
Structure](DEVELOPING.md#project-structure)) rather than reaching into
the subdirectories those headers pull in for you:

```cpp
#include <random>

#include "primitive.hpp"

#include "tfhe/bit.hpp"
#include "tfhe/lift.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/scope.hpp"

// 128-bit-security dimensions (see gate_bootstrap_test.cpp).
using Torus = ModTorus<32>;
using Lwe = lwe_params<tlwe_core_params<Torus, 630>, noise_params<15>>;
using Rlwe = rlwe_params<trlwe_core_params<Torus, 1024>, noise_params<25>>;
using Decomp = dcp_params<16, 7>;

int main() {
  std::mt19937 eng{std::random_device{}()};

  Runtime<Lwe> lwe_runtime(eng);
  Runtime<ParamsPack<Rlwe, Decomp>> rlwe_runtime(eng);

  // Chaining gate outputs needs a Relay too -- see Relay::materialize().
  Circuit<Lwe, Rlwe, Decomp> circuit(
      rlwe_runtime.generate_bootstrap_key<Lwe, Rlwe, Decomp>(
          lwe_runtime.holder().get()));

  // 4 slots, true/false at indices 1/0, matching HomAnd's {0, 1/4}
  // message space.
  Lift<4, Lwe, Rlwe> lift(lwe_runtime);
  Drop<4, Lwe, Rlwe, Decomp> drop(rlwe_runtime);

  Bit<Lwe, Rlwe> a_ct = lift.encrypt(true);
  Bit<Lwe, Rlwe> b_ct = lift.encrypt(false);

  Bit<Lwe, Rlwe> result_ct = circuit.And(a_ct, b_ct);

  bool plaintext = drop.decrypt(result_ct);
}
```

***

## References

- Ilaria Chillotti, Nicolas Gama, Mariya Georgieva, and Malika Izabachène.
  [*TFHE: Fast Fully Homomorphic Encryption over the Torus*](https://eprint.iacr.org/2018/421).
  Cryptology ePrint Archive, Paper 2018/421, 2018.
- Reference TFHE implementation. [*Security and Parameters*](https://tfhe.github.io/tfhe/security_and_params.html) --
  source of the 128-bit-security parameter set (n=630, N=1024) this repository's tests target.
