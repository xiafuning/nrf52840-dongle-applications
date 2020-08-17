// Copyright Steinwurf ApS 2018.
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
/// @brief A complete RLNC decoder including a Payload API.
///
/// This decoder provides the following features:
/// - Linear block decoder using Gauss-Jordan elimination.
/// - Recoding functionality using the previously received symbols
/// - Support for multiple coding vector formats: this stack uses
///   the payload_consumer which specifies the possible formats.
class decoder
{
public:

    /// The callback used to output the logged information
    using log_callback =
        std::function<void(const std::string& zone, const std::string& data)>;

public:

    /// @brief The decoder constructor
    ///
    /// @param field the chosen finite field
    /// @param symbols the number of symbols in a coding block
    /// @param symbol_size the size of a symbol in bytes
    decoder(fifi::finite_field field = fifi::finite_field::binary8,
            uint32_t symbols = 10, uint32_t symbol_size = 1400);

    /// @copydoc layer::reset()
    void reset();

    /// Reconfigures the decoder with the given parameters.
    /// This is useful for reusing an existing coder. Note that the
    /// reconfiguration always implies a reset, so the coder will be in a
    /// clean state after this operation
    ///
    /// @param field the chosen finite field
    /// @param symbols the number of symbols in a coding block
    /// @param symbol_size the size of a symbol in bytes
    void reconfigure(
        fifi::finite_field field, uint32_t symbols, uint32_t symbol_size);

    /// @copydoc layer::field_name()
    fifi::finite_field field_name() const;

    //------------------------------------------------------------------
    // SYMBOL STORAGE API
    //------------------------------------------------------------------

    /// @copydoc layer::symbols()
    uint32_t symbols() const;
    /// @copydoc layer::symbol_size()
    uint32_t symbol_size() const;
    /// @copydoc layer::block_size()
    uint64_t block_size() const;

    /// @copydoc layer::symbol_storage()
    uint8_t* symbol_storage(uint32_t index) const;
    /// @copydoc layer::set_symbol_storage()
    void set_symbol_storage(uint8_t* symbol_storage, uint32_t index);
    /// @copydoc layer::set_symbols_storage()
    void set_symbols_storage(uint8_t* symbols_storage);

    /// @copydoc layer::symbols_initialized()
    uint32_t symbols_initialized() const;
    /// @copydoc layer::is_storage_full()
    bool is_storage_full() const;

    //------------------------------------------------------------------
    // PAYLOAD API
    //------------------------------------------------------------------

    /// @copydoc layer::max_payload_size()
    uint32_t max_payload_size() const;
    /// @copydoc layer::consume_payload()
    void consume_payload(uint8_t* payload);
    /// @copydoc layer::consume_payloads()
    bool consume_payloads(uint8_t** payloads, uint32_t payload_count);
    /// @copydoc layer::produce_payload()
    uint32_t produce_payload(uint8_t* payload);

    //------------------------------------------------------------------
    // DECODER API
    //------------------------------------------------------------------

    /// @copydoc layer::is_complete()
    bool is_complete() const;
    /// @copydoc layer::add_is_complete_callback()
    void add_is_complete_callback(const std::function<void()>& callback);
    /// @copydoc layer::is_partially_complete()
    bool is_partially_complete() const;
    /// @copydoc layer::rank()
    uint32_t rank() const;
    /// @copydoc layer::symbols_missing()
    uint32_t symbols_missing() const;
    /// @copydoc layer::symbols_partially_decoded()
    uint32_t symbols_partially_decoded() const;
    /// @copydoc layer::symbols_decoded()
    uint32_t symbols_decoded() const;
    /// @copydoc layer::is_symbol_missing()
    bool is_symbol_missing(uint32_t index) const;
    /// @copydoc layer::is_symbol_partially_decoded()
    bool is_symbol_partially_decoded(uint32_t index) const;
    /// @copydoc layer::is_symbol_decoded()
    bool is_symbol_decoded(uint32_t index) const;
    /// @copydoc layer::is_symbol_pivot()
    bool is_symbol_pivot(uint32_t index) const;
    /// @copydoc layer::set_symbol_decoded()
    void set_symbol_decoded(uint32_t index);

    /// @copydoc layer::set_status_updater_on()
    void set_status_updater_on();
    /// @copydoc layer::set_status_updater_off()
    void set_status_updater_off();
    /// @copydoc layer::is_status_updater_enabled()
    bool is_status_updater_enabled() const;
    /// @copydoc layer::update_symbol_status()
    void update_symbol_status();

    /// @copydoc layer::remote_rank()
    uint32_t remote_rank() const;

    //------------------------------------------------------------------
    // SYMBOL API
    //------------------------------------------------------------------

    /// @copydoc layer::coefficient_vector_size()
    uint32_t coefficient_vector_size() const;
    /// @copydoc layer::consume_symbol()
    void consume_symbol(uint8_t* symbol_data, uint8_t* coefficients);
    /// @copydoc layer::consume_systematic_symbol()
    void consume_systematic_symbol(uint8_t* symbol_data, uint32_t index);
    /// @copydoc layer::consume_symbols()
    bool consume_symbols(uint8_t** symbol_data, const uint8_t** coefficients,
                         uint32_t symbol_count);

    //------------------------------------------------------------------
    // COEFFICIENT GENERATOR API
    //------------------------------------------------------------------

    /// @copydoc layer::set_seed()
    void set_seed(uint32_t seed_value);
    /// @copydoc layer::generate()
    void generate(uint8_t* coefficients);
    /// @copydoc layer::generate_partial()
    void generate_partial(uint8_t* coefficients);

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
