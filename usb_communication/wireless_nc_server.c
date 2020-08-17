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
    uint8_t rx_buf[MAX_SIZE];
    memset (rx_buf, 0, sizeof rx_buf);
    uint16_t rx_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);
    bool first_frame = true;
    bool first_frame_tail_exist = false;
    bool non_frag_frame_tail_exist = false;
    uint8_t next_frame_size = 0;

    init_reassembler ();

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

    print_nc_config (&decoder);

    // client operations
    while (decoder.is_complete() == false)
    {
        // read from serial port
        if (first_frame == true)
        {
            rx_num = read (fd, rx_buf, MAX_SIZE);
            if (rx_num > 0)
                printf ("receive %d bytes\n", rx_num);
            else if (rx_num == -1)
            {
                fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
                break;
            }
            else // no data received
                continue;
        }
        else if (non_frag_frame_tail_exist == true)
        {
            do
                rx_num += read (fd, rx_buf + rx_num, 1);
            while (rx_num < *rx_buf);
            printf ("receive %d bytes total\n", rx_num);
            non_frag_frame_tail_exist = false;
            first_frame = true;
        }
        else
        {
            rx_num = 0;
            do
                rx_num += read (fd, rx_buf + rx_num, 1);
            while (rx_num < next_frame_size);
            // handle the case that first frame
            // is incomplete in the first rx
            printf ("receive %d bytes\n", rx_num);
            if (first_frame_tail_exist == true)
            {
                next_frame_size = copy_frame_tail (rx_buf, rx_num);
                first_frame_tail_exist = false;
                continue;
            }
        }
        // check if frame is correctly formatted
        if (is_frame_format_correct (rx_buf) == false)
        {
            // if not, reset reassembler
            printf ("incorrect format\n");
            print_payload (rx_buf, rx_num);
            init_reassembler ();
            first_frame = true;
            first_frame_tail_exist = false;
            non_frag_frame_tail_exist = false;
            next_frame_size = 0;
            continue;
        }
        // process received packet
        if (need_reassemble (rx_buf)) // fragmented packet
        {
            if (// last packet is reassembled and receive
                // first frame of a new packet
                (is_reassembler_running () == false &&
                is_first_fragment (rx_buf) == true) ||
                // last packet is in reassembling and receive
                // first frame of a new packet (at least one
                // fragment of last packet is lost)
                (is_reassembler_running () == true &&
                is_new_packet (rx_buf) == true &&
                is_first_fragment (rx_buf) == true &&
                // make sure part of the payload is included
                // avoid segmentation fault
                rx_num > FIRST_FRAG_DATA_OFFSET))
            {
                start_new_reassemble (rx_buf);
                if (first_frame == true)
                    first_frame = false;
                reassembler_t* reassembler = get_reassembler ();
                if (rx_num < reassembler->rx_num_order[0])
                    first_frame_tail_exist = true;
                next_frame_size = read_frame (rx_buf, rx_num);
            }
            // receive a fragment of a known packet
            else if (is_reassembler_running () == true &&
                     is_new_packet (rx_buf) == false)
                next_frame_size = read_frame (rx_buf, rx_num);
            // other cases
            else
            {
                memset (rx_buf, 0, sizeof rx_buf);
                continue;
            }
            if (is_reassemble_complete () == true)
            {
                printf ("packet %u reassemble complete!\n", rx_count);
                extract_packet (extract_buf);
                rx_count++;
                next_frame_size = 0;
                first_frame = true;
                init_reassembler ();
                // read the symbol and coding coefficients into the decoder
                decoder.consume_symbol (extract_buf + decoder.coefficient_vector_size(),
                                        extract_buf);
                memset (extract_buf, 0, sizeof extract_buf);
            }
        }
        else // non-fragmented/normal packet
        {
            if (*rx_buf != rx_num)
            {
                first_frame = false;
                non_frag_frame_tail_exist = true;
                next_frame_size = *rx_buf - rx_num;
                continue;
            }
            printf ("receive a packet\n");
            // read the symbol and coding coefficients into the decoder
            decoder.consume_symbol (rx_buf +
                                    decoder.coefficient_vector_size() +
                                    IPHC_TOTAL_SIZE +
                                    UDPHC_TOTAL_SIZE,
                                    rx_buf +
                                    IPHC_TOTAL_SIZE +
                                    UDPHC_TOTAL_SIZE);
            rx_count++;
        }
        // clear rx buffer after processing
        memset (rx_buf, 0, sizeof rx_buf);
    } // end of while
    printf ("decode complete!\n");
    print_payload (data_out, sizeof data_out);
    printf ("packet total receive: %u\n", rx_count);
    return 0;
}
