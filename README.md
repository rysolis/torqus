# Build Guide

A C++20 TFHE library (leveled arithmetic, gate bootstrapping, and an
end-to-end Client/downstream/Coordinator/Aggregator protocol built on top of it),
compiled either as a native library or as a WebAssembly module.

This project supports:

- Native C++ builds (Clang or GCC)
- WebAssembly (WASM) builds using Emscripten
- An example N-API native Node addon (Downstream only -- see "example N-API Addon Build"
  below for why: it's the one entity that benefits from `seal()`'s
  `std::thread`-based parallelism, which the WASM build can't offer)

The build system is based on CMake and Docker. All commands below are run
from the repository root (this directory is `libs/tfhe`, a subproject of
the monorepo, not a standalone checkout).

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
├── adapter/              # Binding-agnostic entity wrappers + params, shared by wasm/ and example-napi/
├── wasm/                 # Emscripten bindings (Client, downstream, Coordinator, Aggregator)
├── example-napi/                 # example N-API native Node addon (Downstream only, see below)
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
- Node.js + npm -- only for building the example N-API addon locally (not needed
  via Docker, which bundles its own Node.js; see "example N-API Addon Build" below)

***

## Native Build (GCC / Clang)

### 1. Build the Docker image

From the repository root:

```bash
docker build -f libs/tfhe/Dockerfile -t ppv-libs .
```

### 2. Configure & build

Each command mounts the source tree, builds inside the container, and exits
-- no need to stay in an interactive shell.

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
compile for your machine's exact CPU instead of a generic baseline --
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
(`libboost-dev`, header-only) -- both already installed in the image, see
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
invocation -- no rebuild needed:

```bash
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -w /work \
  ppv-libs \
  ctest --test-dir build/gcc/debug
```

By default `test-ppv-lab` builds with `-O2` even under the `gcc-debug`/
`clang-debug` presets (`TFHE_TEST_FAST_DEBUG=ON` in
[`test/CMakeLists.txt`](test/CMakeLists.txt)) -- the bootstrap/keyswitch
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

`encrypt()` samples real Gaussian noise into every ciphertext (param sets
opt in via `noise_params<AlphaBits>`, see `tfhe/params.hpp`); reconfigure
with `-DTFHE_ENABLE_NOISE=OFF` to force it back to exact/noiseless.

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
[`adapter/`](adapter) (binding-agnostic, shared with the example N-API addon below).
The build produces:

```text
build/wasm/release/wasm/tfhe_wasm.js
build/wasm/release/wasm/tfhe_wasm.wasm
build/wasm/release/wasm/tfhe_wasm.d.ts
```

***

## example N-API Addon Build

Downstream specifically -- not Client/Coordinator/Aggregator -- also builds as a
native Node addon ([`example-napi/`](example-napi)) behind an example N-API binding, reusing
[`adapter/downstream.hpp`](adapter/downstream.hpp) unchanged
(it's Emscripten-agnostic, which is why `adapter/` lives outside `wasm/`
and is shared by both `wasm/bindings/` and `example-napi/bindings/`).

It exists because `seal()`'s `ThreadPool`-based parallelism (see
[`include/example/protocol/downstream.hpp`](include/example/protocol/downstream.hpp))
can't run under `tfhe_wasm`: `-pthread` combined with that build's
`-sEXPORT_ES6=1` hangs under Node (emscripten-core/emscripten#8176,
#11278, #18626), and `-sPROXY_TO_PTHREAD` doesn't even link, since it
needs a real `main()` that an embind-only build doesn't have
(emscripten-core/emscripten#9847). Native `std::thread` plus example N-API's
`example-napi::AsyncWorker` (used by
[`example-napi/bindings/downstream.cpp`](example-napi/bindings/downstream.cpp))
sidesteps both.

This build is driven by [`cmake-js`](https://github.com/cmake-js/cmake-js)
via [`example-napi/package.json`](example-napi/package.json), not a CMake preset --
`example-napi/CMakeLists.txt` is its own self-contained project that pulls in
`ppv-libs` (include dirs, `Threads::Threads`, mimalloc) via
`add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/..)`.

**Unlike the WASM build, the result here is a `.node` binary tied to one
platform and CPU architecture** -- `require()` on anything else fails to
load it (`dlopen(...): tried: '...' (slice is not valid mach-o file)` on
macOS for a Linux-built addon). So there are two ways to build it, and
which one to use depends entirely on where `downstream.node` needs to
run:

- **Docker** -- a reproducible build for shipping the addon somewhere
  else (e.g. a Linux production host). Use this when the build machine
  and the machine that runs it are different.
- **Local** -- build directly on this machine, no Docker, no
  `--platform` to think about. Use this when you're building and running
  in the same place, e.g. local development.

### Docker build

Reuses the native build's image (`Dockerfile`, not `Dockerfile.wasm`),
extended with Node.js so `cmake-js` has something to run under:

```bash
docker build -f libs/tfhe/Dockerfile -t ppv-libs .
```

By default this targets whatever architecture the Docker host itself is,
not necessarily where `downstream.node` will actually run. If the
deployment target is a different architecture (e.g. building on this
Apple Silicon Mac for an x86_64 production host, or vice versa), add
`--platform linux/amd64` (or `linux/arm64`) -- that also decides
whether the example N-API addon gets NEON (see "SIMD (ARM NEON)" below) or the
scalar fallback. Building for a platform other than the Docker host's own
needs BuildKit's QEMU-based emulation, on by default in current Docker
Desktop/Engine.

```bash
docker run --rm \
  -v "$(pwd)/libs/tfhe:/work" \
  -v tfhe_example-napi_node_modules:/work/example-napi/node_modules \
  -w /work/example-napi \
  ppv-libs \
  bash -lc "npm install && npm run build"
```

The extra `-v tfhe_example-napi_node_modules:/work/example-napi/node_modules` layers a
named volume on top of the bind mount above, just for `node_modules`.
Without it, every `npm install`'s output would be shadowed right back out
by `-v "$(pwd)/libs/tfhe:/work"` on the next run -- unlike Emscripten for
the WASM build, `cmake-js`/`node-addon-api` are this project's own npm
dependencies rather than baked into the base image, so something has to
install them; the named volume just makes that happen once instead of on
every `docker run`.

This produces:

```text
example-napi/build/Release/downstream.node
```

`npm run build` runs `cmake-js rebuild` (see "SIMD (ARM NEON)" below for
why not `compile`), which downloads/caches the Node headers matching
whatever `node` ran it (see
[`~/.cmake-js`](https://github.com/cmake-js/cmake-js#readme)), configures
`example-napi/CMakeLists.txt`, and builds the `downstream` target.

### Local build

Same command, no Docker -- builds and runs `downstream.node` on this
machine directly:

```bash
cd libs/tfhe/example-napi
npm install
npm run build
```

This needs a C++20 toolchain + CMake 3.22+ on `PATH` (the same
requirements as the native build above), which cmake-js drives directly.
NEON/scalar is picked automatically for whatever this machine is, same as
`cmake --preset clang-native-release` does for the native build above.

### SIMD (ARM NEON)

`Poly`/`Vector` add-sub (`algebra/detail/simd_ops.hpp`) uses ARM NEON
automatically when the target supports it:

```bash
npm run build            # NEON where available (default)
npm run build:no-simd    # force the portable scalar path instead
```

Both scripts run `cmake-js rebuild`, not `cmake-js compile` -- `compile`
only re-runs CMake's configure step when `example-napi/build/` doesn't already
have one, so switching between `npm run build` and `npm run build:no-simd`
on an existing `build/` would otherwise reuse that cache and silently
keep whichever setting was configured first. `rebuild` cleans first every
time, so either script always reflects what you just asked for. The addon
is one source file (`example-napi/bindings/downstream.cpp`), so the full
rebuild this forces is cheap.

ARM NEON is the only vectorized backend today; x86 targets always take the
scalar path (see `simd_lane::ScalarLane`/`WideLane` in
`algebra/detail/simd_ops.hpp`). AVX2/AVX512 support for x86 is wanted --
not implemented yet.
