#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#include "serial.h"
#include "payload.h"
#include "lowpan.h"
#include "reassemble.h"
#include "utils.h"

#include <kodo_rlnc/coders.hpp>

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128

static struct option long_options[] =
{
    {"port",        required_argument, 0, 'p'},
    {"symbolSize",  required_argument, 0, 's'},
    {"genSize",     required_argument, 0, 'g'},
    {"recode",       no_argument,       0, 'r'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-r --recode] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tsymbol size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tgeneration size\t\t\tDefault: 10\n");
    printf ("\t-r --recode\tenable recoding\n");
    printf ("\t-h --help\tthis help documetation\n");
}

void print_nc_config (kodo_rlnc::pure_recoder* recoder)
{
    // print configuration
    printf ("---------NC configuration---------\n");
    printf ("recoder symbol size:\t\t%u\n", recoder->symbol_size());
    printf ("recoder coeff vector size:\t%u\n", recoder->coefficient_vector_size());
    printf ("recoder payload size:\t\t%u\n", recoder->max_payload_size());
    printf ("---------NC configuration---------\n");

}

int main(int argc, char *argv[])
{
    char* serial_port = (char*)USB_DEVICE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;
    bool recode_enable = false;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    while ((opt = getopt_long (argc, argv, "p:s:g:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                serial_port = optarg;
                break;
            case 's':
                symbol_size = atoi (optarg);
                break;
            case 'g':
                generation_size = atoi (optarg);
                break;
            case 'r':
                recode_enable = true;
                break;
            case 'h':
                usage ();
                return 0;
            default:
                usage ();
                return 0;
        }
    }

    // Seed random number generator to produce different results every time
    srand (static_cast<uint32_t> (time (0)));

    // USB communication
    int fd = open_serial_port (serial_port, B115200, 0);
    if (fd < 0)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, serial_port, strerror (errno));
        return -1;
    }

    int ret;
    int rx_num = 0;
    uint16_t rx_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);

    // recoder initialization
    // set finite field size
    fifi::finite_field field = fifi::finite_field::binary8;
    // set the number of symbols/combinations that should be stored in the
    // pure recoder. These coded symbols are combinations of previously
    // received symbols. When a new symbol is received, the pure recoder will
    // combine it with its existing symbols using random coefficients.
    uint32_t recoder_symbols = generation_size;
    // create an recoder
    kodo_rlnc::pure_recoder recoder (field, generation_size, symbol_size, recoder_symbols);
    
    // set buffers
    uint8_t recoder_symbol[recoder.symbol_size()];
    uint8_t recoder_symbol_coefficients[recoder.coefficient_vector_size()];
    // buffer for recoding coefficients
    uint8_t recoder_coefficients[recoder.recoder_coefficient_vector_size()];
    memset (recoder_symbol, 0, sizeof recoder_symbol);
    memset (recoder_symbol_coefficients, 0, sizeof recoder_symbol_coefficients);
    memset (recoder_coefficients, 0, sizeof recoder_coefficients);

    uint32_t payload_length = recoder.symbol_size() + recoder.coefficient_vector_size();
    uint8_t payload[payload_length];
    memset (payload, 0, sizeof payload);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    print_nc_config (&recoder);

    // server operations
    while (true)
    {
        // receive a packet
        rx_num = read_serial_port (fd, extract_buf);
        if (rx_num == 0)
            continue;
        if (recode_enable == true)
        {
            printf ("recode a symbol\n");
            // read symbol and coding coefficients into the recoder
            recoder.consume_symbol (extract_buf + recoder.coefficient_vector_size(),
                                    extract_buf);
            // generate recoding coefficients
            recoder.recoder_generate (recoder_coefficients);
            // write an encoded symbol based on the recoding coefficients
            recoder.recoder_produce_symbol (recoder_symbol,
                                            recoder_symbol_coefficients,
                                            recoder_coefficients);
            // construct payload
            memcpy (payload,
                    recoder_symbol_coefficients,
                    sizeof recoder_symbol_coefficients);
            memcpy (payload + sizeof recoder_symbol_coefficients,
                    recoder_symbol,
                    sizeof recoder_symbol);
            memset (extract_buf, 0, sizeof extract_buf);
            rx_count++;
        }
        else
        {
            // construct payload
            memcpy (payload, extract_buf, payload_length);
        }

        // forward packet
        // fragmentation
        if (need_fragmentation (payload_length) == true)
        {
            printf ("lowpan fragmentation needed\n");
            do_fragmentation (tx_packet, payload, payload_length);
            for (uint8_t j = 0; j < get_fragment_num(); j++)
            {
                ret = write_serial_port (fd, tx_packet[j].packet, tx_packet[j].length);
                if (ret < 0)
                    return -1;
                printf ("forward a frame\n");
            }
        }
        else
        {
            generate_normal_packet (&tx_packet[0], payload, payload_length);
            ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
            if (ret < 0)
                return -1;
            printf ("forward a packet\n");
        }
    } // end of while
    return 0;
}