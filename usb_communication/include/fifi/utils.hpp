// Copyright Steinwurf ApS 2017.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#pragma once

#include <cassert>
#include <cstdint>
#include <initializer_list>

#include "finite_field.hpp"
#include "version.hpp"

#include "fields/binary.hpp"
#include "fields/binary16.hpp"
#include "fields/binary4.hpp"
#include "fields/binary8.hpp"
#include "fields/get_value.hpp"
#include "fields/prime2325.hpp"
#include "fields/set_value.hpp"

#include "fields/bytes_to_elements.hpp"
#include "fields/elements_to_bytes.hpp"

/// The functions defined in this file provide an inline implementation for
/// these frequently used field operations. The optimizer can inline these
/// functions to improve performance.

namespace fifi
{
inline namespace STEINWURF_FIFI_VERSION
{
/// @copydoc math::get_value()
/// @param field the selected finite field
inline uint32_t get_value(
    fifi::finite_field field, const uint8_t* elements, uint32_t index)
{
    switch (field)
    {
    case fifi::finite_field::binary:
        return fields::get_value(fields::binary(), elements, index);
    case fifi::finite_field::binary4:
        return fields::get_value(fields::binary4(), elements, index);
    case fifi::finite_field::binary8:
        return fields::get_value(fields::binary8(), elements, index);
    case fifi::finite_field::binary16:
        return fields::get_value(fields::binary16(), elements, index);
    case fifi::finite_field::prime2325:
        return fields::get_value(fields::prime2325(), elements, index);
    default:
        assert(0 && "Unknown field");
        return 0U;
    }
}

/// @copydoc math::set_value()
/// @param field the selected finite field
inline void set_value(
    fifi::finite_field field, uint8_t* elements, uint32_t index, uint32_t value)
{
    switch (field)
    {
    case fifi::finite_field::binary:
        fields::set_value(fields::binary(), elements, index, value);
        break;
    case fifi::finite_field::binary4:
        fields::set_value(fields::binary4(), elements, index, value);
        break;
    case fifi::finite_field::binary8:
        fields::set_value(fields::binary8(), elements, index, value);
        break;
    case fifi::finite_field::binary16:
        fields::set_value(fields::binary16(), elements, index, value);
        break;
    case fifi::finite_field::prime2325:
        fields::set_value(fields::prime2325(), elements, index, value);
        break;
    default:
        assert(0 && "Unknown field");
        break;
    }
}

/// Sets a number of field values, for example:
///
///    std::vector(uint32_t> data(4);
///    fifi::set_values(fifi::finite_field::binary8, data.data(), {0, 1, 1, 0});
///
/// @param field The selected finite field
/// @param elements Pointer to the data buffer where the values should be
///        initialized
/// @param values The actual values to use
inline void set_values(
    fifi::finite_field field, uint8_t* elements,
    const std::initializer_list<uint32_t> values)
{
    assert(values.size() > 0);
    uint32_t i = 0;

    for (auto v : values)
    {
        set_value(field, elements, i, v);
        ++i;
    }
}

/// @copydoc math::elements_to_bytes()
/// @param field the selected finite field
inline uint32_t elements_to_bytes(fifi::finite_field field, uint32_t elements)
{
    switch (field)
    {
    case fifi::finite_field::binary:
        return fields::elements_to_bytes(fields::binary(), elements);
    case fifi::finite_field::binary4:
        return fields::elements_to_bytes(fields::binary4(), elements);
    case fifi::finite_field::binary8:
        return fields::elements_to_bytes(fields::binary8(), elements);
    case fifi::finite_field::binary16:
        return fields::elements_to_bytes(fields::binary16(), elements);
    case fifi::finite_field::prime2325:
        return fields::elements_to_bytes(fields::prime2325(), elements);
    default:
        assert(0 && "Unknown field");
        return 0U;
    }
}

/// @copydoc math::bytes_to_elements()
/// @param field the selected finite field
inline uint32_t bytes_to_elements(fifi::finite_field field, uint32_t size)
{
    switch (field)
    {
    case fifi::finite_field::binary:
        return fields::bytes_to_elements(fields::binary(), size);
    case fifi::finite_field::binary4:
        return fields::bytes_to_elements(fields::binary4(), size);
    case fifi::finite_field::binary8:
        return fields::bytes_to_elements(fields::binary8(), size);
    case fifi::finite_field::binary16:
        return fields::bytes_to_elements(fields::binary16(), size);
    case fifi::finite_field::prime2325:
        return fields::bytes_to_elements(fields::prime2325(), size);
    default:
        assert(0 && "Unknown field");
        return 0U;
    }
}
}
}
