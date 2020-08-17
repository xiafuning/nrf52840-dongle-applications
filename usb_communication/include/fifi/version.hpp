// Copyright (c) Steinwurf ApS 2019.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#pragma once

#include <string>

namespace fifi
{
/// Here we define the STEINWURF_FIFI_VERSION this should be updated on each
/// release
#define STEINWURF_FIFI_VERSION v30_5_1

inline namespace STEINWURF_FIFI_VERSION
{
/// @return The version of the library as string
std::string version();
}
}