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
/// The finite fields
enum class finite_field: uint8_t
{
    /// The binary field
    binary,
    /// The binary extension field with 2⁴ elements
    binary4,
    /// The binary extension field with 2⁸ elements
    binary8,
    /// The binary extension field with 2¹⁶ elements
    binary16,
    /// Prime field with 2³²-5 elements
    prime2325
};
}
}
