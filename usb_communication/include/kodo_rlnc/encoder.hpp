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

#include "coding_vector_format.hpp"

#include "version.hpp"

namespace kodo_rlnc
{
inline namespace STEINWURF_KODO_RLNC_VERSION
{
/// @ingroup fec_stacks
///
/// @brief A complete RLNC encoder including a Payload API.
///
/// The key features of this configuration are the following:
/// - Systematic encoding (uncoded symbols produced before switching
///   to coded symbols)
/// - Multiple options for the encoding vector format: this stack uses
///   the payload_producer which specifies the possible formats.
/// - By default, a seed is sent instead of the full encoding vectors,
///   this reduces the amount of overhead per symbol.
/// - Encoding vectors are generated using a random uniform generator.
class encoder
{
public:

    /// The callback used to output the logged information
    using log_callback =
        std::function<void(const std::string& zone, const std::string& data)>;

public:

    /// @brief The encoder constructor
    ///
    /// @param field the chosen finite field
    /// @param symbols the number of symbols in a coding block
    /// @param symbol_size the size of a symbol in bytes
    encoder(fifi::finite_field field = fifi::finite_field::binary8,
            uint32_t symbols = 10, uint32_t symbol_size = 1400);

    /// @copydoc layer::reset()
    void reset();

    /// Reconfigures the encoder with the given parameters.
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

    /// Set the coding vector format
    /// @param format The selected coding vector format
    void set_coding_vector_format(kodo_rlnc::coding_vector_format format);

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
    const uint8_t* symbol_storage(uint32_t index) const;
    /// @copydoc layer::set_symbol_storage()
    void set_symbol_storage(const uint8_t* symbol_storage, uint32_t index);
    /// @copydoc layer::set_symbols_storage()
    void set_symbols_storage(const uint8_t* symbols_storage);

    /// @copydoc layer::symbols_initialized()
    uint32_t symbols_initialized() const;
    /// @copydoc layer::is_storage_full()
    bool is_storage_full() const;

    //------------------------------------------------------------------
    // PAYLOAD API
    //------------------------------------------------------------------

    /// @copydoc layer::max_payload_size()
    uint32_t max_payload_size() const;
    /// @copydoc layer::produce_payload()
    uint32_t produce_payload(uint8_t* payload);
    /// @copydoc layer::produce_payloads()
    uint32_t produce_payloads(uint8_t** payloads, uint32_t payload_count);

    //------------------------------------------------------------------
    // ENCODER API
    //------------------------------------------------------------------

    /// @copydoc layer::rank()
    uint32_t rank() const;
    /// @copydoc layer::is_systematic_on()
    bool is_systematic_on() const;
    /// @copydoc layer::set_systematic_on()
    void set_systematic_on();
    /// @copydoc layer::set_systematic_off()
    void set_systematic_off();
    /// @copydoc layer::in_systematic_phase()
    bool in_systematic_phase() const;

    //------------------------------------------------------------------
    // SYMBOL API
    //------------------------------------------------------------------

    /// @copydoc layer::coefficient_vector_size()
    uint32_t coefficient_vector_size() const;
    /// @copydoc layer::produce_symbol()
    uint32_t produce_symbol(uint8_t* symbol_data, uint8_t* coefficients);
    /// @copydoc layer::produce_systematic_symbol()
    uint32_t produce_systematic_symbol(uint8_t* symbol_data, uint32_t index);
    /// @copydoc layer::produce_symbols()
    void produce_symbols(uint8_t** symbol_data, const uint8_t** coefficients,
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

    /// @copydoc layer::set_density()
    void set_density(float density);
    /// @copydoc layer::density()
    float density() const;

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
