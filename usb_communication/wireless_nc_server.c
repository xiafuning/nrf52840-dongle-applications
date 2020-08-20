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
#include "utils.h"
#include "lowpan.h"
#include "reassemble.h"
#include "config.h"

#include <kodo_rlnc/coders.hpp>

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128

static struct option long_options[] =
{
    {"port",        required_argument, 0, 'p'},
    {"symbolSize",  required_argument, 0, 's'},
    {"genSize",     required_argument, 0, 'g'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tsymbol size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tgeneration size\t\t\tDefault: 10\n");
    printf ("\t-h --help\tthis help documetation\n");
}

void print_nc_config (kodo_rlnc::decoder* decoder)
{
    // print configuration
    printf ("---------NC configuration---------\n");
    printf ("decoder symbol size:\t\t%u\n", decoder->symbol_size());
    printf ("decoder coeff vector size:\t%u\n", decoder->coefficient_vector_size());
    printf ("decoder payload size:\t\t%u\n", decoder->max_payload_size());
    printf ("---------NC configuration---------\n");

}

int main(int argc, char *argv[])
{
    char* serial_port = (char*)USB_DEVICE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;

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

    // decoder initialization
    // set finite field size
    fifi::finite_field field = fifi::finite_field::binary8;
    // create an decoder
    kodo_rlnc::decoder decoder (field, generation_size, symbol_size);

    // set buffers
    uint8_t data_out[decoder.block_size()];
    memset (data_out, 0, sizeof data_out);
    // assign source data buffer to encoder
    decoder.set_symbols_storage (data_out);

    // set ack message buffer
    virtual_packet_t ack_buf;
    memset (&ack_buf, 0, sizeof ack_buf);
    generate_normal_packet (&ack_buf, (uint8_t*)SERVER_ACK, sizeof SERVER_ACK);

    print_nc_config (&decoder);

    // server operations
    while (decoder.is_complete() == false)
    {
        // receive a packet
        rx_num = read_serial_port (fd, extract_buf);
        if (rx_num == 0)
            continue;
        // read symbol and coding coefficients into the decoder
        decoder.consume_symbol (extract_buf + decoder.coefficient_vector_size(),
                                extract_buf);
        memset (extract_buf, 0, sizeof extract_buf);
        rx_count++;
    } // end of while
    printf ("decode complete!\n");
    // send ACK message
    ret = write_serial_port (fd, ack_buf.packet, ack_buf.length);
    if (ret < 0)
        return -1;

    print_payload (data_out, sizeof data_out);
    printf ("packet total receive: %u\n", rx_count);
    return 0;
}
