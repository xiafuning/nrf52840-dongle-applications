// Copyright Steinwurf ApS 2015.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <string>

#include <fifi/finite_field.hpp>

#include "version.hpp"

namespace kodo_rlnc
{
inline namespace STEINWURF_KODO_RLNC_VERSION
{
/// @ingroup fec_stacks
///
/// @brief Implementation of a pure RLNC recoder that will not decode the
/// incoming data, it will only re-encode it.
///
/// The "set_recoder_symbols" method of the factory can be used to set the
/// number of symbols/combinations that should be stored in the pure recoder.
/// These coded symbols are combinations of previously received symbols. When
/// a new symbol is received, the pure recoder will combine it with its
/// existing symbols using random coefficients.
class pure_recoder
{
public:

    /// The callback used to output the logged information
    using log_callback =
        std::function<void(const std::string& zone, const std::string& data)>;

public:

    /// @brief The recoder constructor
    ///
    /// @param field the chosen finite field
    /// @param symbols the number of symbols in a coding block
    /// @param symbol_size the size of a symbol in bytes
    /// @param recoder_symbols the number of internal symbols stored in the
    ///        pure recoder. If this parameter is not specified or the value
    ///        is set to zero, then by default "recoder_symbols" will be equal
    ///        to "symbols".
    pure_recoder(fifi::finite_field field = fifi::finite_field::binary8,
                 uint32_t symbols = 10, uint32_t symbol_size = 1400,
                 uint32_t recoder_symbols = 0);

    /// @copydoc layer::reset()
    void reset();

    /// Reconfigures the recoder with the given parameters.
    /// This is useful for reusing an existing coder. Note that the
    /// reconfiguration always implies a reset, so the coder will be in a
    /// clean state after this operation
    ///
    /// @param field the chosen finite field
    /// @param symbols the number of symbols in a coding block
    /// @param symbol_size the size of a symbol in bytes
    /// @param recoder_symbols the number of internal symbols stored in the
    ///        pure recoder.
    void reconfigure(
        fifi::finite_field field, uint32_t symbols, uint32_t symbol_size,
        uint32_t recoder_symbols);

    /// @copydoc layer::field_name()
    fifi::finite_field field_name() const;

    //------------------------------------------------------------------
    // SYMBOL STORAGE API
    //------------------------------------------------------------------

    /// @copydoc layer::recoder_symbols()
    uint32_t recoder_symbols() const;

    /// @copydoc layer::symbols()
    uint32_t symbols() const;
    /// @copydoc layer::symbol_size()
    uint32_t symbol_size() const;
    /// @copydoc layer::block_size()
    uint64_t block_size() const;

    //------------------------------------------------------------------
    // PAYLOAD API
    //------------------------------------------------------------------

    /// @copydoc layer::max_payload_size()
    uint32_t max_payload_size() const;
    /// @copydoc layer::consume_payload()
    void consume_payload(uint8_t* payload);
    /// @copydoc layer::produce_payload()
    uint32_t produce_payload(uint8_t* payload);

    //------------------------------------------------------------------
    // SYMBOL API
    //------------------------------------------------------------------

    /// @copydoc layer::coefficient_vector_size()
    uint32_t coefficient_vector_size() const;
    /// @copydoc layer::consume_symbol()
    void consume_symbol(uint8_t* symbol_data, uint8_t* coefficients);
    /// @copydoc layer::recoder_coefficient_vector_size()
    uint32_t recoder_coefficient_vector_size() const;
    /// @copydoc layer::recoder_produce_symbol()
    uint32_t recoder_produce_symbol(
        uint8_t* recoded_symbol_data, uint8_t* recoded_symbol_coefficients,
        uint8_t* recoding_coefficients);

    /// @copydoc layer::set_symbol_storage()
    void set_symbol_storage(const uint8_t* symbol_storage, uint32_t index);
    /// @copydoc layer::set_coefficient_storage()
    void set_coefficient_storage(const uint8_t* coefficients, uint32_t index);

    //------------------------------------------------------------------
    // COEFFICIENT GENERATOR API
    //------------------------------------------------------------------

    /// @copydoc layer::generate()
    void recoder_generate(uint8_t* coefficients);

    //------------------------------------------------------------------
    // LOG API
    //------------------------------------------------------------------

    /// @copydoc layer::set_log_callback()
    void set_log_callback(const log_callback& callback);
    /// @copydoc layer::set_log_stdout()
    void set_log_stdout();
    /// @copydoc layer::set_log_off()
    void set_log_off();
    /// @copydoc layer::set_zone_prefix()
    void set_zone_prefix(const std::string& zone_prefix);
    /// @copydoc layer::is_log_enabled()
    bool is_log_enabled() const;

private:

    // Private implemenation
    struct impl;
    std::shared_ptr<impl> m_impl;
};

}
}
