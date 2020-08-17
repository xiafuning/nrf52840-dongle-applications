// Copyright (c) Steinwurf ApS 2019.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#pragma once

#include <cstdint>

#include "version.hpp"

namespace fifi
{
inline namespace STEINWURF_FIFI_VERSION
{
/// The SIMD acceleration
enum class simd: uint8_t
{
    /// No Acceleration
    none,
    /// SSE2 Acceleration
    sse2,
    /// SSSE3 Acceleration
    ssse3,
    /// SSE4.2 Acceleration
    sse42,
    /// AVX2 Acceleration
    avx2,
    /// NEON Acceleration
    neon
};
}
}
