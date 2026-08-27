# torqus

The name is a portmanteau of *torque* (the rotational force behind TFHE's
blind rotation) and *torus* (the ring group 𝕋 that TFHE computes over).

A C++20, header-only TFHE library (leveled arithmetic, gate bootstrapping).
Application-level protocols built on top of it -- along with their
WASM/example N-API bindings and npm packaging -- live downstream (e.g. in
`ppv-lab`), not in this repository.

This project supports:

- Native C++ builds (Clang or GCC)

The build system is based on CMake and Docker. All commands below are run
from the repository root.

***

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
│   ├── algebra/          # polynomials, vectors, ring arithmetic
│   ├── arithmetic/       # expression traits, negacyclic convolution
│   ├── primitive/        # modint, torus, uint, and their concepts
│   └── tfhe/
│       ├── cryptor/      # TLWE / TRLWE / TRGSW cryptors
│       ├── structure/    # ciphertext & key types
│       ├── operation/    # leveled ops, gate bootstrapping
│       ├── gate/         # homomorphic gates (AND, AND-NOT, ...)
│       ├── circuit/      # gate-level circuits (e.g. binary expansion)
│       └── serialize/    # wire (de)serialization
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
docker build -f Dockerfile -t torqus-libs .
```

### 2. Configure & build

Each command mounts the source tree, builds inside the container, and exits
-- no need to stay in an interactive shell.

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

Optionally, on Linux, `gcc-native-debug` / `gcc-native-release` /
`clang-native-debug` / `clang-native-release` add `-march=native` to
compile for your machine's exact CPU instead of a generic baseline --
faster, but the binary is tied to the build machine's CPU features, so
don't ship it or use it in CI. Unlike the commands above, this runs directly
on your machine (not inside the container):

```bash
cmake --preset clang-native-release && cmake --build --preset clang-native-release -j$(nproc)
```

### 3. Run tests (GoogleTest)

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
define `NDEBUG`, so `assert(bound < 0.25)` and friends stay active either
way. If you need to single-step through a test in a debugger, where `-O0`
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

***

## Packaging

The C++ core (`include/`) is header-only and packaged for two C++ package
managers under the name `torqus`.

### Conan

[`conanfile.py`](conanfile.py) packages `include/` behind `find_package(torqus)`
(`torqus::torqus`), matching the `CMakeLists.txt` install/export section below.
`mimalloc`/SIMD/noise are exposed as Conan options (`use_mimalloc`,
`enable_simd`, `enable_noise`), mirroring the `TORQUS_USE_MIMALLOC` /
`TORQUS_ENABLE_SIMD` / `TFHE_ENABLE_NOISE` CMake options above:

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

A downstream consumer that wants a WebAssembly or native Node (example N-API) build
of an application built on top of `torqus` (e.g. `ppv-lab`) links against
this same header-only library from their own Emscripten/`cmake-js` build --
this repository doesn't produce one itself.
