# Build Guide

A C++20 TFHE library (leveled arithmetic, gate bootstrapping, and an
end-to-end Client/downstream/Coordinator/Aggregator protocol built on top of it),
compiled either as a native library or as a WebAssembly module.

This project supports both:

- Native C++ builds (Clang or GCC)
- WebAssembly (WASM) builds using Emscripten

The build system is based on CMake and Docker. All commands below are run from the repository root (this directory is `libs/tfhe`, a subproject of the monorepo, not a standalone checkout).

***

## Project Structure

```text
libs/tfhe/
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
├── Dockerfile.wasm
├── include/
│   ├── algebra/          # polynomials, vectors, ring arithmetic
│   ├── arithmetic/       # expression traits, negacyclic convolution
│   ├── primitive/        # modint, torus, uint, and their concepts
│   ├── tfhe/
│   │   ├── cryptor/      # TLWE / TRLWE / TRGSW cryptors
│   │   ├── structure/    # ciphertext & key types
│   │   ├── operation/    # leveled ops, gate bootstrapping
│   │   ├── gate/         # homomorphic gates (AND, AND-NOT, ...)
│   │   ├── circuit/      # gate-level circuits (e.g. binary expansion)
│   │   └── serialize/    # wire (de)serialization
│   └── e2e/
│       ├── encoding/     # message <-> plaintext codec
│       └── protocol/     # Client / downstream / Coordinator / Aggregator
├── wasm/                 # Emscripten bindings (Client, downstream, Coordinator, Aggregator)
├── test/
│   ├── compile/          # compile-only interface/contract tests
│   ├── runtime/          # GoogleTest runtime tests
│   └── test.cpp
└── README.md
```

***

## Requirements

- Docker
- CMake 3.22+
- GNU Make

***

## Native Build (GCC / Clang)

### 1. Build the Docker image

From the repository root:

```bash
docker build -f libs/tfhe/Dockerfile -t ppv-libs .
```

### 2. Configure & build

Each command mounts the source tree, builds inside the container, and exits
— no need to stay in an interactive shell.

```bash
# GCC
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs \
  bash -lc "cmake --preset gcc-release && cmake --build --preset gcc-release -j\$(nproc)"

# Clang
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs \
  bash -lc "cmake --preset clang-release && cmake --build --preset clang-release -j\$(nproc)"
```

Optionally, on Linux, `gcc-native-debug` / `gcc-native-release` /
`clang-native-debug` / `clang-native-release` add `-march=native` to
compile for your machine's exact CPU instead of a generic baseline —
faster, but the binary is tied to the build machine's CPU features, so
don't ship it or use it in CI. Unlike the commands above, this runs directly
on your machine (not inside the container), from `libs/tfhe`:

```bash
cd libs/tfhe
cmake --preset clang-native-release && cmake --build --preset clang-native-release -j$(nproc)
```

### 3. Run tests (GoogleTest)

Tests use GoogleTest (`libgtest-dev`) and the noise-bound tracking in
`tfhe/utility/analysis/` uses Boost.Rational / Boost.Multiprecision
(`libboost-dev`, header-only) — both already installed in the image, see
[`Dockerfile`](Dockerfile). The image also has `libmimalloc-dev`; CMake
links it into every executable automatically when present (`PPV_USE_MIMALLOC`
in [`CMakeLists.txt`](CMakeLists.txt)), replacing the system allocator.
Build the debug preset once:

```bash
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs \
  bash -lc "cmake --preset gcc-debug && cmake --build --preset gcc-debug -j\$(nproc)"
```

The test binary is now sitting in `build/gcc/debug` on the mounted host
directory, so running (and re-running) it is just a plain container
invocation — no rebuild needed:

```bash
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs \
  ctest --test-dir build/gcc/debug
```

By default `test-ppv-lab` builds with `-O2` even under the `gcc-debug`/
`clang-debug` presets (`TFHE_TEST_FAST_DEBUG=ON` in
[`test/CMakeLists.txt`](test/CMakeLists.txt)) — the bootstrap/keyswitch
runtime tests are unusably slow at `-O0`, and Debug's default flags don't
define `NDEBUG`, so `assert(bound < 0.25)` and friends stay active either
way. If you need to single-step through a test in a debugger, where `-O0`
keeps variables inspectable, reconfigure with the flag off:

```bash
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs \
  cmake --preset gcc-debug -DTFHE_TEST_FAST_DEBUG=OFF
```

***

## WebAssembly Build (Emscripten)

### 1. Build the Docker image

From the repository root:

```bash
docker build -f libs/tfhe/Dockerfile.wasm -t ppv-libs:wasm .
```

### 2. Configure & build

```bash
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs:wasm \
  bash -lc "emcmake cmake --preset wasm-release && cmake --build --preset wasm-release -j\$(nproc)"
```

`emcmake` automatically configures CMake to use:

```text
CMAKE_CXX_COMPILER=em++
```

This builds the `tfhe_wasm` target (see [`wasm/CMakeLists.txt`](wasm/CMakeLists.txt)),
which embind-wraps the Client/downstream/Coordinator/Aggregator adapters under
[`wasm/adapter/`](wasm/adapter). The build produces:

```text
build/wasm/release/wasm/tfhe_wasm.js
build/wasm/release/wasm/tfhe_wasm.wasm
build/wasm/release/wasm/tfhe_wasm.d.ts
```
