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
    {"redundancy",  required_argument, 0, 'r'},
    {"density",     no_argument,       0, 'd'},
    {"recode",      no_argument,       0, 'c'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-r --redundancy <redundancy in percent>] [-d --density] [-c --recode] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tsymbol size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tgeneration size\t\t\tDefault: 10\n");
    printf ("\t-r --redundancy\tredundancy in percent\t\tDefault: 20\n");
    printf ("\t-d --density\tenable sparse coding\n");
    printf ("\t-c --recode\tenable recoding\n");
    printf ("\t-h --help\tthis help documetation\n");
}

void print_nc_config (kodo_rlnc::encoder* encoder,
                      float redundancy,
                      uint16_t total_tx_num)
{
    // print configuration
    printf ("---------NC configuration---------\n");
    printf ("encoder symbol size:\t\t%u\n", encoder->symbol_size());
    printf ("encoder coeff vector size:\t%u\n", encoder->coefficient_vector_size());
    printf ("encoder payload size:\t\t%u\n", encoder->max_payload_size());
    printf ("redundancy ratio:\t\t%.2f\n", redundancy);
    printf ("total tx packets:\t\t%u\n", total_tx_num);
    printf ("---------NC configuration---------\n");
}

int main(int argc, char *argv[])
{
    // cmd argument related variables
    char* serial_port = (char*)USB_DEVICE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;
    float redundancy = 0.2;
    bool sparse_enable = false;
    bool recode_enable = false;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    while ((opt = getopt_long (argc, argv, "p:s:g:r:dch", long_options, &option_index)) != -1)
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
                redundancy = (float)atoi (optarg) / 100;
                break;
            case 'd':
                sparse_enable = true;
                break;
            case 'c':
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

    // variable definitions
    uint32_t inter_frame_interval = 30000; // inter frame interval in us
    int ret;
    uint8_t rx_buf[MAX_SIZE];
    memset (rx_buf, 0, sizeof rx_buf);
    uint16_t tx_frame_count = 0;
    uint16_t tx_packet_count = 0;
    uint16_t total_tx_num = 0;
    if (recode_enable == false)
        total_tx_num = generation_size * (1 + redundancy);
    else
        total_tx_num = generation_size * (1 + redundancy / 2);
    // time related variables
    struct timeval send_start, send_end;
    memset (&send_start, 0, sizeof send_start);
    memset (&send_end, 0, sizeof send_end);
    uint64_t total_time_used = 0; // us

    // encoder initialization
    // set finite field size
    fifi::finite_field field = fifi::finite_field::binary8;
    // create an encoder
    kodo_rlnc::encoder encoder (field, generation_size, symbol_size);

    // enable sparse coding
    if (sparse_enable == true)
        encoder.set_density (0.5);

    // set buffers
    uint8_t encoder_symbol[encoder.symbol_size()];
    uint8_t encoder_symbol_coefficients[encoder.coefficient_vector_size()];
    uint8_t data_in[encoder.block_size()];
    memset (encoder_symbol, 0, sizeof encoder_symbol);
    memset (encoder_symbol_coefficients, 0, sizeof encoder_symbol_coefficients);
    // fill source data buffer with specific values
    memset (data_in, 'T', sizeof (data_in));

    // set ack packet buffer
    uint8_t ack_packet[MAX_SIZE];
    memset (ack_packet, 0, sizeof ack_packet);

    // assign source data buffer to encoder
    encoder.set_symbols_storage (data_in);

    // set data packet buffer
    uint32_t tx_packet_length = 0;
    uint8_t packet[encoder.symbol_size() +
                   encoder.coefficient_vector_size() +
                   IPHC_TOTAL_SIZE +
                   UDPHC_TOTAL_SIZE];
    memset (packet, 0, sizeof packet);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    print_nc_config (&encoder, redundancy, total_tx_num);

    // client operations
    while (tx_packet_count < total_tx_num)
    {
        if (tx_packet_count < generation_size) // systematic phase
        {
            // generate systematic symbol
            encoder.produce_systematic_symbol (encoder_symbol, tx_packet_count);
            // construct packet
            set_ip_header (packet);
            set_udp_header (packet + IPHC_TOTAL_SIZE);
            // set index of systematic packet
            *(packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE) = (uint8_t)tx_packet_count;
            memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE + sizeof (uint8_t),
                    encoder_symbol,
                    sizeof encoder_symbol);
            tx_packet_length = IPHC_TOTAL_SIZE +
                               UDPHC_TOTAL_SIZE +
                               sizeof (uint8_t) +
                               sizeof encoder_symbol;
        }
        else // coding phase
        {
            // generate coding coefficients for an encoded packet
            encoder.generate (encoder_symbol_coefficients);
            // write encoded symbol corresponding to the coding coefficients
            encoder.produce_symbol (encoder_symbol,
                                    encoder_symbol_coefficients);
            // construct packet
            set_ip_header (packet);
            set_udp_header (packet + IPHC_TOTAL_SIZE);
            memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                    encoder_symbol_coefficients,
                    sizeof encoder_symbol_coefficients);
            memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE +
                    sizeof encoder_symbol_coefficients,
                    encoder_symbol,
                    sizeof encoder_symbol);
            tx_packet_length = IPHC_TOTAL_SIZE +
                               UDPHC_TOTAL_SIZE +
                               sizeof encoder_symbol_coefficients +
                               sizeof encoder_symbol;
        }

        // mark start time
        gettimeofday (&send_start, NULL);

        // fragmentation
        if (need_fragmentation (tx_packet_length) == true)
        {
            printf ("[client] lowpan fragmentation needed\n");
            do_fragmentation (tx_packet, packet, tx_packet_length);
            for (uint8_t j = 0; j < get_fragment_num(); j++)
            {
                ret = write_serial_port (fd, tx_packet[j].packet, tx_packet[j].length);
                if (ret < 0)
                    return -1;
                printf ("[client] send a frame\n");
                tx_frame_count++;
                usleep (inter_frame_interval);
            }
            tx_packet_count++;
        }
        else
        {
            generate_normal_packet (&tx_packet[0], packet, tx_packet_length);
            ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
            if (ret < 0)
                return -1;
            printf ("[client] send a packet\n");
            tx_frame_count++;
            tx_packet_count++;
            usleep (inter_frame_interval);
        }

        // mark end time
        gettimeofday (&send_end, NULL);
        total_time_used += 1000000 * (send_end.tv_sec - send_start.tv_sec) +
                           send_end.tv_usec - send_start.tv_usec;
    } // end of while
    printf ("[client] packet total send: %u\n", tx_packet_count);
    printf ("[client] frame total send: %u\n", tx_frame_count);
    return 0;
}
