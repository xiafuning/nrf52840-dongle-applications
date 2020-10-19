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

#define USB_DEVICE  "/dev/ttyACM0"
#define LOG_FILE    "log.dump"
#define MAX_SIZE    128

static struct option long_options[] =
{
    {"port",        required_argument, 0, 'p'},
    {"symbolSize",  required_argument, 0, 's'},
    {"genSize",     required_argument, 0, 'g'},
    {"redundancy",  required_argument, 0, 'r'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-r --redundancy <redundancy in percent>] [-l --logFile <log file name>][-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tsymbol size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tgeneration size\t\t\tDefault: 10\n");
    printf ("\t-r --redundancy\tredundancy in percent\t\tDefault: 20\n");
    printf ("\t-l --logFile\tlog file name\t\t\tDefault: log.dump\n");
    printf ("\t-h --help\tthis help documetation\n");
}

void print_nc_config (kodo_rlnc::decoder* decoder,
                      float redundancy)
{
    // print configuration
    printf ("---------NC configuration---------\n");
    printf ("decoder symbol size:\t\t%u\n", decoder->symbol_size());
    printf ("decoder coeff vector size:\t%u\n", decoder->coefficient_vector_size());
    printf ("decoder payload size:\t\t%u\n", decoder->max_payload_size());
    printf ("redundancy ratio:\t\t%.2f\n", redundancy);
    printf ("---------NC configuration---------\n");
}

int write_measurement_log (char* log_file_name,
                           kodo_rlnc::decoder* decoder,
                           float redundancy)
{
    FILE* fp;
    float loss_rate = 0;
    fp = fopen (log_file_name, "a+");
    if (fp == NULL)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, log_file_name, strerror (errno));
        return -1;
    }
    if (decoder->is_complete() == false)
        loss_rate = (float)(decoder->symbols() - decoder->rank()) / decoder->symbols();

    fprintf(fp, "{\"type\": \"block_full\", \"gen_size\": %u, \"redundancy\": %.2f, \"loss_rate\": %.2f, \"tx_num\": %u },\n",
            decoder->symbols(),
            redundancy,
            loss_rate,
            (unsigned)(decoder->symbols() * (1 + redundancy)));
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    char* serial_port = (char*)USB_DEVICE;
    char* log_file_name = (char*)LOG_FILE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;
    float redundancy = 0.2;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    while ((opt = getopt_long (argc, argv, "p:s:g:r:l:h", long_options, &option_index)) != -1)
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
            case 'l':
                log_file_name = optarg;
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
    uint16_t rx_packet_count = 0;
    uint16_t rx_frame_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);

    // time related variable definition
    clock_t rx_timeout_start = 0;
    uint32_t rx_timeout = 1000; // ms, hard coded

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
    uint8_t ack_packet[MAX_SIZE];
    memset (ack_packet, 0, sizeof ack_packet);
    memset (&ack_buf, 0, sizeof ack_buf);
    // construct ack packet
    uint8_t ack_packet_length = generate_ack_packet (ack_packet, (uint8_t*)SERVER_ACK);
    generate_normal_packet (&ack_buf, ack_packet, ack_packet_length);

    print_nc_config (&decoder, redundancy);

    rx_timeout_start = clock();
    // server operations
    while (decoder.is_complete() == false)
    {
        if ((clock() - rx_timeout_start) * 1000 / CLOCKS_PER_SEC > rx_timeout)
            break;
        // receive a packet
        rx_num = read_serial_port (fd, extract_buf, &rx_frame_count);
        if (rx_num == 0)
            continue;
        else if ((unsigned)rx_num == IPHC_TOTAL_SIZE +
                                     UDPHC_TOTAL_SIZE +
                                     decoder.coefficient_vector_size() +
                                     decoder.symbol_size())
        // receive a coded packet
        {
            print_payload (extract_buf, rx_num);
            // read symbol and coding coefficients into the decoder
            decoder.consume_symbol (extract_buf + decoder.coefficient_vector_size() +
                                    IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                                    extract_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE);
            rx_packet_count++;
        }
        else if ((unsigned)rx_num == IPHC_TOTAL_SIZE +
                                     UDPHC_TOTAL_SIZE +
                                     sizeof (uint8_t) +
                                     decoder.symbol_size())
        // receive a systematic packet
        {
            decoder.consume_systematic_symbol
                (extract_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE + sizeof (uint8_t),
                *(extract_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE));
            rx_packet_count++;
        }
        memset (extract_buf, 0, sizeof extract_buf);
    } // end of while

    print_payload (data_out, sizeof data_out);

    if (decoder.is_complete() == true)
        printf ("decode complete!\n");
    else
        printf ("decode failure!\n");

    printf ("decoder rank: %u\n", decoder.rank());
    printf ("packet total receive: %u\n", rx_packet_count);
    printf ("frame total receive: %u\n", rx_frame_count);

    // write log to json file
    ret = write_measurement_log (log_file_name, &decoder, redundancy);
    if (ret < 0)
        return -1;
    return 0;
}
