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

void print_nc_config (kodo_rlnc::encoder* encoder)
{
    // print configuration
    printf ("---------NC configuration---------\n");
    printf ("encoder symbol size:\t\t%u\n", encoder->symbol_size());
    printf ("encoder coeff vector size:\t%u\n", encoder->coefficient_vector_size());
    printf ("encoder payload size:\t\t%u\n", encoder->max_payload_size());
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

    uint32_t inter_frame_interval = 100000; // inter frame interval in us
    int ret;
    bool decode_complete_ack = false;
    int rx_num = 0;
    uint8_t rx_buf[MAX_SIZE];
    memset (rx_buf, 0, sizeof rx_buf);
    uint16_t tx_count = 0;

    // encoder initialization
    // set finite field size
    fifi::finite_field field = fifi::finite_field::binary8;
    // create an encoder
    kodo_rlnc::encoder encoder (field, generation_size, symbol_size);

    // set buffers
    uint8_t encoder_symbol[encoder.symbol_size()];
    uint8_t encoder_symbol_coefficients[encoder.coefficient_vector_size()];
    uint8_t data_in[encoder.block_size()];
    memset (encoder_symbol, 0, sizeof encoder_symbol);
    memset (encoder_symbol_coefficients, 0, sizeof encoder_symbol_coefficients);
    // fill source data buffer with specific values
    memset (data_in, 'T', sizeof (data_in));

    // assign source data buffer to encoder
    encoder.set_symbols_storage (data_in);

    uint32_t payload_length = encoder.symbol_size() + encoder.coefficient_vector_size();
    uint8_t payload[payload_length];
    memset (payload, 0, sizeof payload);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    print_nc_config (&encoder);

    // client operations
    while (decode_complete_ack == false)
    {
        // generate coding coefficients for an encoded packet
        encoder.generate (encoder_symbol_coefficients);
        // write encoded symbol corresponding to the coding coefficients
        encoder.produce_symbol (encoder_symbol,
                                encoder_symbol_coefficients);
        // construct payload
        memcpy (payload,
                encoder_symbol_coefficients,
                sizeof encoder_symbol_coefficients);
        memcpy (payload + sizeof encoder_symbol_coefficients,
                encoder_symbol,
                sizeof encoder_symbol);
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
                printf ("send a frame\n");
                usleep (inter_frame_interval);
            }
            tx_count++;
        }
        else
        {
            generate_normal_packet (&tx_packet[0], payload, payload_length);
            ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
            if (ret < 0)
                return -1;
            printf ("send a packet\n");
            tx_count++;
            usleep (inter_frame_interval);
        }
        // check for ack from decoder
        rx_num = read_serial_port (fd, rx_buf);
        if (rx_num > 0 &&
            strcmp ((const char*)rx_buf, SERVER_ACK) == 0)
        {
            printf ("receive ACK from decoder\n");
            decode_complete_ack = true;
            // send ack to relay node
            generate_normal_packet (&tx_packet[0],
                                    (uint8_t*)CLIENT_ACK,
                                    sizeof CLIENT_ACK);
            ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
            if (ret < 0)
                return -1;
        }
        else if (rx_num == -1)
        {
            fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
            break;
        }
    }
    printf ("packet total send: %u\n", tx_count);
    return 0;
}
