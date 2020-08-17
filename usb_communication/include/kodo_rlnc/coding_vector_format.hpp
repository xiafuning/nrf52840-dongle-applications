// Copyright Steinwurf ApS 2018.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#pragma once

#include <cstdint>

#include "version.hpp"

namespace kodo_rlnc
{
inline namespace STEINWURF_KODO_RLNC_VERSION
{
/// Enum to select a coding vector format for an encoder or decoder.
enum class coding_vector_format: uint8_t
{
    /// Send the full encoding vector, i.e. all coding coefficients with
    /// every encoded symbol. With this format, the amount of overhead depends
    /// on the number of symbols in a generation.
    full_vector,
    /// Only send a 4-byte random seed instead of the full encoding vectors.
    /// The seed represents the current state of the random number generator,
    /// so the same coefficients can be generated on the decoder side.
    /// This format gives us a fixed amount of overhead per symbol, but
    /// it disables the recoding functionality.
    seed,
    /// In addition to the 4-byte random seed, this format also includes the
    /// density of the coding vector as a 4-byte float. A density-based random
    /// generator is used to control the number of non-zero elements.
    /// This format also gives us a fixed amount of overhead per symbol, but
    /// it disables the recoding functionality.
    sparse_seed
};
}
}
