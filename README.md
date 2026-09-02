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
- [Developing torqus](#developing-torqus)
  - [Performance & Concurrency](#performance--concurrency)
  - [Project Structure](#project-structure)
  - [Requirements](#requirements)
  - [Build & Run the Test Suite (Docker)](#build--run-the-test-suite-docker)
  - [Building Natively, Without Docker](#building-natively-without-docker)
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
root (see [Project Structure](#project-structure)); each row links to
the umbrella header that pulls in its subdirectory.

| Category | Types / Operations | Header |
| --- | --- | --- |
| Ciphertexts | `TLWE`, `TRLWE`, `TRGSW` | [`tfhe/ciphertext.hpp`](include/tfhe/ciphertext.hpp) |
| Leveled arithmetic | `Add`, `Sub`, `KeySwitch`, `SampleExtract` | [`tfhe/operation.hpp`](include/tfhe/operation.hpp) |
| Bootstrap primitives | `BlindRotate`, `ExternalProduct`, `CMux`, `GateBootstrap` | [`tfhe/operation.hpp`](include/tfhe/operation.hpp) |
| Gates | `HomAnd`, `HomAndNot`, `HomOr` | [`tfhe/gate.hpp`](include/tfhe/gate.hpp) |
| Circuits | `BinaryExpansion` (gate-level binary expansion) | [`tfhe/circuit.hpp`](include/tfhe/circuit.hpp) |
| Serialization | wire (de)serialization for the ciphertext/key types above | [`tfhe/serialize.hpp`](include/tfhe/serialize.hpp) |

Gates take Lwe-shaped ciphertexts in and return a fresh Rlwe-domain
ciphertext out (see the `HomAnd` example below); chaining several
together, as `BinaryExpansion` does, needs a `KeySwitch` back down to
Lwe between calls.

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
see [Developing torqus](#developing-torqus) below).

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
Concurrency](#performance--concurrency)) only covers 32-bit elements
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

-- or, vendoring as a git submodule instead, `add_subdirectory(path/to/torqus)`
in place of the `FetchContent` block above does the same thing.
`PROJECT_IS_TOP_LEVEL` in this repository's own `CMakeLists.txt` keeps
its own test suite (GoogleTest, Boost, mimalloc, ...) from being pulled
into your build even if your project also has `BUILD_TESTING` on.

### Example: a homomorphic AND gate

Encrypt two bits under a `Runtime`, combine them, and call
[`GateBootstrap`](include/tfhe/operation/bootstrap/gate_bootstrap.hpp)
directly to get the AND back out as a fresh ciphertext -- this is the same
recipe [`tfhe::gate::HomAnd`](include/tfhe/gate/hom_and.hpp) itself uses,
spelled out so the bootstrapping step isn't hidden behind it. Everything
below comes in through the umbrella headers at `tfhe/`'s root (see
[Project Structure](#project-structure)) rather than reaching into the
subdirectories those headers pull in for you:

```cpp
#include <random>

#include "primitive.hpp"

#include "tfhe/ciphertext.hpp"
#include "tfhe/operation.hpp"
#include "tfhe/params.hpp"
#include "tfhe/runtime.hpp"
#include "tfhe/utility/testvector.hpp"

// The reference TFHE implementation's published 128-bit-security
// dimensions, with their matching lattice-estimator-verified noise
// (alpha = 2^-15 for n=630, 2^-25 for N=1024) -- see gate_bootstrap_test.cpp.
using Torus = ModTorus<32>;
using Lwe = lwe_params<tlwe_core_params<Torus, 630>, noise_params<15>>;
using Rlwe = rlwe_params<trlwe_core_params<Torus, 1024>, noise_params<25>>;
using Decomp = dcp_params<16, 7>;

int main() {
  std::mt19937 eng{std::random_device{}()};

  Runtime<Lwe> lwe_runtime(eng);
  Runtime<ParamsPack<Rlwe, Decomp>> rlwe_runtime(eng);

  // Bootstrap key: lets the Rlwe-side runtime bootstrap ciphertexts
  // encrypted under the Lwe-side secret.
  auto bk = rlwe_runtime.generate_bootstrap_key<Lwe, Rlwe, Decomp>(
      lwe_runtime.holder().get());

  // Message space here is {0, 1/4}: 0 = false, 1/4 = true.
  TLWE<Torus, Lwe::n> a = lwe_runtime.encrypt(Torus(1u, 4u));  // true
  TLWE<Torus, Lwe::n> b = lwe_runtime.encrypt(Torus(0u));      // false

  // AND(a, b) = GateBootstrap(a + b - 1/8): shifting the sum by -1/8
  // makes the phase's sign match "a AND b"; GateBootstrap re-encrypts
  // that sign as a fresh {0, 1/4} ciphertext.
  TLWE<Torus, Lwe::n> offset;
  offset.b() = -Torus(1u, 8u);
  TLWE<Torus, Lwe::n> combined = tfhe::leveled::Add<Lwe>::exec_impl(
      offset, tfhe::leveled::Add<Lwe>::exec_impl(a, b));

  TRLWE<Torus, Rlwe::N> tv;
  tv.b() = testvector::generate<Torus, Rlwe::N>(Torus(1u, 8u));

  TLWE<Torus, Rlwe::N> result =
      tfhe::bootstrap::GateBootstrap<Lwe, Rlwe, Decomp>::exec_impl(
          Torus(1u, 4u), tv, combined, bk);

  // Real Gaussian noise is enabled (noise_params above), so this lands
  // close to Torus(0u) but not exactly on it.
  Torus plaintext = rlwe_runtime.decrypt(result);
}
```

***

## Developing torqus

The sections below are for building and testing this repository itself --
they're not needed just to *use* torqus as a library; see
[Usage](#usage) above for that.

### Performance & Concurrency

Poly/Vector add-sub -- and so every ciphertext type built on them
(TLWE/TRLWE/TRGSW's own `+`/`-`) -- use ARM NEON when available
(`algebra/detail/simd_ops.hpp`), falling back to scalar elsewhere; on by
default, this can be switched off by defining `TORQUS_DISABLE_SIMD`
before including any torqus header (in this repository's own CMake
build, `-DTORQUS_ENABLE_SIMD=OFF` does the same).

Separately, a fixed-size `ThreadPool` (`tfhe/utility/thread_pool.hpp`) is
available but not yet wired into any operation in this repository --
usable as-is by a downstream implementation wanting to batch independent
gate/ciphertext work across threads. Beyond SIMD add-sub, further
computational acceleration (GPU, ...) is intentionally left to
downstream forks -- this repository optimizes for
the correctness and readability of the reference implementation, not for
raw throughput.

### Project Structure

```text
torqus/
├── CMakeLists.txt
├── CMakePresets.json
├── conanfile.py          # Conan recipe (package name "torqus")
├── vcpkg.json            # vcpkg manifest (package name "torqus")
├── cmake/
│   └── torqusConfig.cmake.in   # find_package(torqus) support
├── Dockerfile
├── include/
│   ├── algebra.hpp       # umbrella: Poly / Vector ring arithmetic
│   ├── primitive.hpp     # umbrella: modint, torus, uint
│   ├── algebra/          # polynomials, vectors, ring arithmetic (detail/: SIMD, negacyclic convolution)
│   ├── primitive/        # modint, torus, uint, and their concepts
│   └── tfhe/
│       ├── params.hpp          # parameter types (tlwe_core_params, dcp_params, noise_params, ...)
│       ├── feature.hpp         # Tracking feature tag
│       ├── runtime.hpp         # Runtime<Params, Feature...> facade (encrypt/decrypt, key generation)
│       ├── public_runtime.hpp  # PublicRuntime: encrypt via a PublicKey, no secret needed
│       ├── ciphertext.hpp      # -> structure/ciphertext/: TLWE / TRLWE / TRGSW
│       ├── key.hpp             # -> structure/key/: bootstrap / key-switch / public keys
│       ├── operation.hpp       # -> operation/: leveled ops, bootstrap primitives
│       ├── gate.hpp            # -> gate/: homomorphic gates (AND, AND-NOT, ...)
│       ├── circuit.hpp         # -> circuit/: gate-level circuits (e.g. binary expansion)
│       ├── math.hpp            # -> math/: modulus switching
│       ├── serialize.hpp       # -> serialize/: wire (de)serialization
│       ├── transport.hpp       # direct vs. serialized hand-off between protocol roles
│       ├── cryptor.hpp         # Cryptor<Params, Feature...>, used internally by Runtime
│       ├── concept/            # tlwe_concept/trlwe_concept/... constraints on Params types
│       └── utility/            # noise tracking, secret holder, random generator, ...
├── test/
│   ├── compile/          # compile-only interface/contract tests
│   ├── runtime/          # GoogleTest runtime tests
│   └── test.cpp
└── README.md
```

### Requirements

- Docker
- CMake 3.22+
- GNU Make

### Build & Run the Test Suite (Docker)

#### 1. Build the Docker image

From the repository root:

```bash
docker build -f Dockerfile -t torqus-libs .
```

#### 2. Configure & build

Each command mounts the source tree, builds inside the container, and exits
-- no need to stay in an interactive shell. `torqus-libs` itself is an
INTERFACE (header-only) CMake target with nothing to compile, so the one
concrete thing these commands produce is `test-torqus`, the test suite
binary used in step 3 below.

```bash
# GCC
docker run --rm \
  -v "$(pwd):/work" \
  -w /work \
  torqus-libs \
  bash -lc "cmake --preset gcc-release && cmake --build --preset gcc-release -j\$(nproc)"

# Clang
docker run --rm \
  -v "$(pwd):/work" \
  -w /work \
  torqus-libs \
  bash -lc "cmake --preset clang-release && cmake --build --preset clang-release -j\$(nproc)"
```

#### 3. Run tests (GoogleTest)

Tests use GoogleTest (`libgtest-dev`) and the noise-bound tracking in
`tfhe/utility/analysis/` uses Boost.Rational / Boost.Multiprecision /
Boost.Math (`libboost-dev`, header-only) -- all already installed in the image, see
[`Dockerfile`](Dockerfile). The image also has `libmimalloc-dev`; CMake
links it into every executable automatically when present (`TORQUS_USE_MIMALLOC`
in [`CMakeLists.txt`](CMakeLists.txt)), replacing the system allocator.
Build the debug preset once:

```bash
docker run --rm \
  -v "$(pwd):/work" \
  -w /work \
  torqus-libs \
  bash -lc "cmake --preset gcc-debug && cmake --build --preset gcc-debug -j\$(nproc)"
```

The test binary is now sitting in `build/gcc/debug` on the mounted host
directory, so running (and re-running) it is just a plain container
invocation -- no rebuild needed:

```bash
docker run --rm \
  -v "$(pwd):/work" \
  -w /work \
  torqus-libs \
  ctest --test-dir build/gcc/debug
```

By default `test-torqus` builds with `-O2` even under the `gcc-debug`/
`clang-debug` presets (`TFHE_TEST_FAST_DEBUG=ON` in
[`test/CMakeLists.txt`](test/CMakeLists.txt)) -- the bootstrap/keyswitch
runtime tests are unusably slow at `-O0`, and Debug's default flags don't
define `NDEBUG`, so invariant checks like `assert(other.size() ==
this->size())` (`algebra/vector.hpp`) stay active either way. If you need
to single-step through a test in a debugger, where `-O0`
keeps variables inspectable, reconfigure with the flag off:

```bash
docker run --rm \
  -v "$(pwd):/work" \
  -w /work \
  torqus-libs \
  cmake --preset gcc-debug -DTFHE_TEST_FAST_DEBUG=OFF
```

`encrypt()` samples real Gaussian noise into every ciphertext (param sets
opt in via `noise_params<AlphaBits>`, see `tfhe/params.hpp`); reconfigure
with `-DTFHE_ENABLE_NOISE=OFF` to force it back to exact/noiseless.

CI also builds/tests `gcc-debug-torus32`/`clang-debug-torus32` and
`gcc-debug-torus64`/`clang-debug-torus64` the same way, so both
`TORQUS_TORUS_BITS` widths stay covered. Any other preset can be built at
either width too, by adding `-DTORQUS_TORUS_BITS=64` at configure time
(see [Quick Start](#quick-start)):

```bash
cmake --preset gcc-release -DTORQUS_TORUS_BITS=64
```

### Building Natively, Without Docker

`gcc-native-debug` / `gcc-native-release` / `clang-native-debug` /
`clang-native-release` skip Docker entirely and run directly on your own
machine, adding `-march=native` to compile for your machine's exact CPU
instead of a generic baseline -- faster, but the binary is tied to the
build machine's CPU features, so don't ship it or use it in CI. They
still require the same toolchain/dependencies the Docker image installs
(GoogleTest, Boost, ...) to be present on your host. To keep such a
binary from being mistaken for the portable `test-torqus` built by the
presets above, these presets set `TORQUS_NATIVE_BUILD` (`test/CMakeLists.txt`),
which renames the output to `test-torqus-native-${CMAKE_SYSTEM_PROCESSOR}`
(e.g. `test-torqus-native-arm64`):

```bash
cmake --preset clang-native-release && cmake --build --preset clang-native-release -j$(nproc)
```

***

## References

- Ilaria Chillotti, Nicolas Gama, Mariya Georgieva, and Malika Izabachène.
  [*TFHE: Fast Fully Homomorphic Encryption over the Torus*](https://eprint.iacr.org/2018/421).
  Cryptology ePrint Archive, Paper 2018/421, 2018.
- Reference TFHE implementation. [*Security and Parameters*](https://tfhe.github.io/tfhe/security_and_params.html) --
  source of the 128-bit-security parameter set (n=630, N=1024) this repository's tests target.
