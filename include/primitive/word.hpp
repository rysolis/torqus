// Copyright 2026 Ryuhei Morita
// SPDX-License-Identifier: Apache-2.0

#ifndef PRIMITIVE_WORD_HPP
#define PRIMITIVE_WORD_HPP

#include <cstdint>

#ifndef TORQUS_TORUS_BITS
#define TORQUS_TORUS_BITS 32
#endif

#if TORQUS_TORUS_BITS != 32 && TORQUS_TORUS_BITS != 64
#error "TORQUS_TORUS_BITS must be 32 or 64"
#endif

// The default raw storage word for UInt and ModTorus<QBit, Word>, selected
// by the TORQUS_TORUS_BITS CMake option (see CMakeLists.txt). Naming a
// wider or narrower Word explicitly at a given ModTorus<QBit, Word>
// instantiation always overrides this default -- this only decides what
// happens when a caller doesn't ask.
#if TORQUS_TORUS_BITS == 64
using torqus_default_word_t = uint64_t;
#else
using torqus_default_word_t = uint32_t;
#endif

#endif  // PRIMITIVE_WORD_HPP
