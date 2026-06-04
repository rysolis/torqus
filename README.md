# Build Guide

This project supports both:

- Native C++ builds (Clang or GCC)
- WebAssembly (WASM) builds using Emscripten

The build system is based on CMake and Docker.

---

## Project Structure

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── Dockerfile
├── Dockerfile.wasm
├── include/
│   ├── algebra/
│   │   └── poly.hpp
│   ├── arithmetic/
│   ├── primitive/
│   │   ├── modint.hpp
│   │   ├── scalar.hpp
│   │   ├── torus.hpp
│   │   └── uint.hpp
│   └── tfhe/
│   │   ├── cryptor/
│   │   ├── operation/
│   │   └── structure/
│   └── encoding/
├── test/
│   ├── compile/
│   ├── runtime/
│   └── test.cpp
└── README.md
```

---

## Requirements

- Docker
- CMake 3.28+
- GNU Make

---

## Build Docker Images

From the project root directory:

### Clang / GCC

```bash
docker build -f Dockerfile -t ppv-libs .
```

### WASM

```bash
docker build -f Dockerfile.wasm -t ppv-libs:wasm .
```

---

## Run Containers

It is recommended to mount the project directory as a volume.

### Clang / GCC

```bash
docker run --rm -it \
  -v $(pwd):/work \
  -w /work \
  -v /etc/passwd:/etc/passwd:ro \
  -v /etc/group:/etc/group:ro \
  -u $(id -g) \
  ppv-libs /bin/bash
```

### WASM

```bash
docker run --rm -it \
  -v $(pwd):/work \
  -w /work \
  -v /etc/passwd:/etc/passwd:ro \
  -v /etc/group:/etc/group:ro \
  -u $(id -g) \
  ppv-libs:wasm /bin/bash
```

---

## Native Build

Inside the native container:

### GCC

```bash
cmake --preset gcc-release
cmake --build --preset gcc-release -j
```

### Clang (WIP)

```bash
cmake --preset clang-release
cmake --build --preset clang-release -j
```

---

## WebAssembly Build (WIP)

Inside the WASM container:

```bash
emcmake cmake --preset wasm-release
cmake --build --preset wasm-release -j
```

`emcmake` automatically configures CMake to use:

```text
CMAKE_CXX_COMPILER=em++
```

---

## Running the WASM Build

Work in progress.

---

## Running Tests

Configure the project:

```bash
cmake --preset gcc-debug
```

Build all targets:

```bash
cmake --build --preset gcc-debug -j
```

Run the test suite:

```bash
ctest --test-dir build/gcc/debug
```