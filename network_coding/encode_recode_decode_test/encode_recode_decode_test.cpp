// Copyright Steinwurf ApS 2015.
// Distributed under the "STEINWURF EVALUATION LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing

/// @example pure_recode_symbol_api.cpp
///
/// This example is very similar to encode_recode_decode_simple.cpp.
/// The only difference is that this example uses a pure recoder instead of
/// a decoder acting as a recoder. By "pure", we mean that the recoder will not
/// decode the incoming data, it will only re-encode it.
/// This example shows how to use an encoder, recoder, and decoder to
/// simulate a simple relay network as shown below. For simplicity,
/// we have error free links, i.e. no data packets are lost when being
/// sent from encoder to recoder to decoder:
///
///         +-----------+      +-----------+      +------------+
///         |  encoder  |+---->| recoder   |+---->|  decoder   |
///         +-----------+      +-----------+      +------------+
///
/// For a more elaborate description of recoding, please see the
/// description of encode_recode_decode_simple.cpp.

#include <algorithm>
#include <iostream>
#include <vector>
#include <fstream>

#include <kodo_rlnc/coders.hpp>


/**
 * @brief function for opening a file
 */
int create_file(const std::string &filename, std::fstream* data_stream)
{
    //open file
    data_stream->open(filename, std::ios::out | std::ios::trunc);
    if (data_stream->is_open())
    {
        std::cout << "file is open" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "error: file not open" << std::endl;
        return -1;
    }
}

/**
 * @brief function for doing coding
 */
uint32_t run_coding(kodo_rlnc::encoder& encoder,
                kodo_rlnc::pure_recoder& recoder,
                kodo_rlnc::decoder& decoder, 
                float packet_loss_rate)
{
    uint32_t tx_num = 0;
    uint8_t recoder_symbol_num = 0;
    bool recoder_ack = false;
    
    // disable systematic coding
    encoder.set_systematic_off();

    // Allocate the needed buffers:
    //
    // 1. We need a buffer for the encoded symbol and the coding coeffcients
    //    used to encode the symbol.
    //
    std::vector<uint8_t> encoder_symbol (encoder.symbol_size());
    std::vector<uint8_t> encoder_symbol_coefficients (encoder.coefficient_vector_size());

    // 2. For the recoder we need a buffer to contain a recoded symbol and the
    //    recoded coding coefficients.
    //
    std::vector<uint8_t> recoder_symbol (recoder.symbol_size());
    std::vector<uint8_t> recoder_symbol_coefficients (recoder.coefficient_vector_size());

    // 3. For the recoder we also need a buffer for the recoding coefficients.
    //    The number of recoding coefficients we will have depends on the number
    //    of recoding symbols. We do not have to worry about that here we can
    //    just query the stack what the size should be.
    //
    std::vector<uint8_t> recoder_coefficients (recoder.recoder_coefficient_vector_size());

    // Allocate some data to encode. In this case we make a buffer
    // with the same size as the encoder's block size (the max.
    // amount a single encoder can encode)
    std::vector<uint8_t> data_in (encoder.block_size());

    // Just for fun - fill the data with random data
    std::generate (data_in.begin(), data_in.end(), rand);

    
    // Install the default trace function on all coders (writes to std::cout)
    //encoder.set_log_stdout();
    //encoder.set_zone_prefix ("encoder");
    //recoder.set_log_stdout();
    //recoder.set_zone_prefix ("recoder");
    //decoder.set_log_stdout();
    //decoder.set_zone_prefix ("decoder");
    

    //std::cout << "Setting encoder symbol storage =>" << std::endl << std::endl;

    // Assign the data buffer to the encoder so that we may start
    // to produce encoded symbols from it
    encoder.set_symbols_storage (data_in.data());

    //std::cout << "Setting decoder symbol storage =>" << std::endl << std::endl;

    // Define a data buffer where the symbols should be decoded
    std::vector<uint8_t> data_out (decoder.block_size());
    decoder.set_symbols_storage (data_out.data());

    while (!decoder.is_complete())
    {
        // Both the encoding and recoding vector can be manually specified, e.g.
        // like so:
        //
        //     fifi::set_values<field_type>(encoder_symbol_coefficients.data(),
        //         {16, 34, 1, 0, 8, 0});

        if (recoder_ack == false)
        {
            // Generate the coding coefficients for an encoded packet
            encoder.generate (encoder_symbol_coefficients.data());

            // Write the encoded symbol corresponding to the coding coefficients
            encoder.produce_symbol (encoder_symbol.data(),
                                    encoder_symbol_coefficients.data());

            // source -> intermediate
            tx_num++;
            if ((rand() % 100) < (int)(packet_loss_rate * 100))
                continue;

            // Read the symbol and coding coefficients into the recoder
            recoder.consume_symbol (encoder_symbol.data(),
                                    encoder_symbol_coefficients.data());
            recoder_symbol_num++;
            if (recoder_symbol_num == recoder.recoder_symbols())
                recoder_ack = true;
        }
        // Generate the recoding coefficients
        recoder.recoder_generate (recoder_coefficients.data());

        // Write an encoded symbol based on the recoding coefficients
        recoder.recoder_produce_symbol (recoder_symbol.data(),
                                        recoder_symbol_coefficients.data(),
                                        recoder_coefficients.data());

        // intermediate -> destination
        tx_num++;
        if ((rand() % 100) < (int)(packet_loss_rate * 100))
            continue;

        // Read the symbol and coding coefficients into the decoder
        decoder.consume_symbol (recoder_symbol.data(),
                                recoder_symbol_coefficients.data());
    }

    // Check if we properly decoded the data
    if (std::equal (data_out.begin(), data_out.end(), data_in.begin()))
    {
        //std::cout << "Data decoded correctly" << std::endl;
        return tx_num;
    }
    else
    {
        std::cout << "Unexpected failure to decode "
                  << "please file a bug report :)" << std::endl;
        return 0;
    }
}

/**
 * @brief main function
 */
int main()
{
    // create a file
    std::string filename = "/home/xiafuning/diplom-arbeit/src/kodo-rlnc/examples/encode_recode_decode_test/log.dump";
    std::fstream log;
    int ret = create_file(filename, &log);
    if (ret != 0)
        return -1;

    // Seed random number generator to produce different results every time
    srand (static_cast<uint32_t> (time (0)));
    
    // parameter configuration
    uint32_t generation_size = 10;
    uint32_t symbol_size = 70;
    float packet_loss_rate = 0.1;

    uint32_t rounds = 100;
    uint32_t tx_num = 0;
    uint32_t average_tx_num = 0;

    fifi::finite_field field = fifi::finite_field::binary8;

    // Set the number of symbols/combinations that should be stored in the
    // pure recoder. These coded symbols are combinations of previously
    // received symbols. When a new symbol is received, the pure recoder will
    // combine it with its existing symbols using random coefficients.
    uint32_t recoder_symbols = generation_size;

    // Create encoder, recoder and decoder
    kodo_rlnc::encoder encoder (field, generation_size, symbol_size);
    kodo_rlnc::pure_recoder recoder (field, generation_size, symbol_size, recoder_symbols);
    kodo_rlnc::decoder decoder (field, generation_size, symbol_size);

    // print configuration
    std::cout << "------------configuration------------" << std::endl;
    std::cout << "encoder symbol size:\t\t\t" << encoder.symbol_size() << std::endl;
    std::cout << "encoder coeff vector size:\t\t" << encoder.coefficient_vector_size() << std::endl;
    std::cout << "encoder payload size:\t\t\t" << encoder.max_payload_size() << std::endl;
    
    std::cout << "recoder symbol size:\t\t\t" << recoder.symbol_size() << std::endl;
    std::cout << "recoder coeff vector size:\t\t" << recoder.coefficient_vector_size() << std::endl;
    std::cout << "recoder recoding coeff vector size:\t" << recoder.recoder_coefficient_vector_size() << std::endl;
    std::cout << "recoder payload size:\t\t\t" << recoder.max_payload_size() << std::endl;
    std::cout << "------------configuration------------" << std::endl << std::endl;

    // run simulations
    log << "[" << std::endl;
    for (float loss_rate = 0.1; loss_rate < 0.8; loss_rate += 0.2)
    {
        for (uint32_t i = 0; i < rounds; i++)
        {
            tx_num = run_coding (encoder, recoder, decoder, loss_rate);
            if (tx_num > 0)
            {
                average_tx_num += tx_num;
                //std::cout << "round " << i << " complete" << std::endl;
            }
            encoder.reset();
            recoder.reset();
            decoder.reset();
        }
        std::cout << "packet loss rate: " << loss_rate << ", average number of transmission: " << (float)average_tx_num / rounds << std::endl;
        log << "{\"type\": \"block_full\", \"loss_rate\": " << loss_rate << ", \"average_tx_num\": " << (float)average_tx_num / rounds << " }," << std::endl;
        average_tx_num = 0;
    }
    log << "]" << std::endl;
    return 0;
}
