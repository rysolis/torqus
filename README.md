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
(AND, AND-NOT, ...) with static noise tracking.

</div>

**Concept:** a cryptography library a mathematician can actually read
and a developer can actually ship -- types and primitives named close
to the TFHE paper's own notation (`TLWE`/`TRLWE`/`TRGSW`, `n`/`N`,
`alpha`, ...) rather than hidden behind opaque byte buffers, so each
piece of code traces directly back to the paper construct it
implements -- engineered to the correctness and rigor a production
deployment needs, not just a research prototype.

Gate bootstrapping is designed against the TFHE paper's own published
128-bit-security parameter set (n=630, N=1024), exercised in
[`gate_bootstrap_test.cpp`](test/runtime/tfhe/operation/bootstrap/gate_bootstrap_test.cpp)
and required to clear a 99% two-sided decryption-success threshold --
*assuming* the accumulated ciphertext error is normally distributed, the
field's standard modeling assumption, not a proven hard bound.
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

## Table of Contents

- [Security Status](#security-status)
- [Supported Operations](#supported-operations)
- [Usage](#usage)
  - [Quick Start](#quick-start)
  - [Conan](#conan)
  - [vcpkg](#vcpkg)
  - [CMake `find_package(torqus)`](#cmake-find_packagetorqus)
  - [Example: a homomorphic AND gate](#example-a-homomorphic-and-gate)
- [Developing torqus](#developing-torqus)
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

Beyond the plain-clone Quick Start above, vendoring `include/` as a git
submodule works the same way. For dependency-managed integration
instead, the C++ core is also packaged for two package managers under
the name `torqus`:

### Conan

[`conanfile.py`](conanfile.py) packages `include/` behind `find_package(torqus)`
(`torqus::torqus`), matching the `CMakeLists.txt` install/export section below.
`mimalloc`/SIMD/noise are exposed as Conan options (`use_mimalloc`,
`enable_simd`, `enable_noise`), mirroring the `TORQUS_USE_MIMALLOC` /
`TORQUS_ENABLE_SIMD` / `TFHE_ENABLE_NOISE` CMake options below:

```bash
conan create . --build=missing
```

### vcpkg

[`vcpkg.json`](vcpkg.json) is the package manifest (name `torqus`, a
`mimalloc` feature on by default). Building it as a vcpkg **port** needs a
`portfile.cmake` as well, which lives in a vcpkg registry rather than in
this repository -- see the project's vcpkg registry setup for that piece.

### CMake `find_package(torqus)`

Either path above ultimately relies on this repository's own `install()`
rules (`CMakeLists.txt`) plus [`cmake/torqusConfig.cmake.in`](cmake/torqusConfig.cmake.in),
which a consumer can also drive directly:

```bash
cmake --preset clang-native-release
cmake --build --preset clang-native-release
cmake --install build/clang-native/release --prefix /some/prefix
```

```cmake
find_package(torqus CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE torqus::torqus)
```

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

// The TFHE paper's own published 128-bit-security dimensions, with their
// matching lattice-estimator-verified noise (alpha = 2^-15 for n=630,
// 2^-25 for N=1024) -- see gate_bootstrap_test.cpp.
using Torus = ModTorus<32>;
using Lwe = lwe_params<tlwe_core_params<Torus, 630>, noise_params<15>>;
using Rlwe = rlwe_params<trlwe_core_params<Torus, 1024>, noise_params<25>>;
using Decomp = dcp_params<256, 3>;

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
`tfhe/utility/analysis/` uses Boost.Rational / Boost.Multiprecision
(`libboost-dev`, header-only) -- both already installed in the image, see
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
