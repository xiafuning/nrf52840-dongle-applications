// Copyright (c) Steinwurf ApS 2019.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#pragma once

#include <string>

namespace kodo_rlnc
{
/// Here we define the STEINWURF_KODO_RLNC_VERSION this should be updated on
/// each release
#define STEINWURF_KODO_RLNC_VERSION v16_1_1

inline namespace STEINWURF_KODO_RLNC_VERSION
{
/// @return The version of the library as string
std::string version();
}
}