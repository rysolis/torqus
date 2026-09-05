# Developing torqus

This document is for building and testing this repository itself --
it's not needed just to *use* torqus as a library; see the
[README](README.md#usage) for that.

- [Performance & Concurrency](#performance--concurrency)
- [Project Structure](#project-structure)
- [Requirements](#requirements)
- [Build & Run the Test Suite (Docker)](#build--run-the-test-suite-docker)
- [Building Natively, Without Docker](#building-natively-without-docker)

***

## Performance & Concurrency

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

## Project Structure

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
│       ├── gate.hpp            # -> gate/: homomorphic gates (AND, AND-NOT, OR, XOR)
│       ├── dial.hpp            # Dial<Resolution, Torus>: names a Torus value by slot index
│       ├── bit.hpp             # Bit<Lwe, Rlwe>: hides a ciphertext's Lwe/Rlwe shape
│       ├── scope.hpp           # Circuit/Relay: gate calls and materializing as method calls
│       ├── lift.hpp            # Lift/Drop: encrypt-only / decrypt-only Runtime wrappers
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

## Requirements

- Docker
- CMake 3.22+
- GNU Make

## Build & Run the Test Suite (Docker)

### 1. Build the Docker image

From the repository root:

```bash
docker build -f Dockerfile -t torqus-libs .
```

### 2. Configure & build

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

### 3. Run tests (GoogleTest)

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
(see [Quick Start](README.md#quick-start)):

```bash
cmake --preset gcc-release -DTORQUS_TORUS_BITS=64
```

## Building Natively, Without Docker

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
