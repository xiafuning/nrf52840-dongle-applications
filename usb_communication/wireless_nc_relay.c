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
#include <signal.h>

#include "serial.h"
#include "lowpan.h"
#include "reassemble.h"
#include "utils.h"
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
    {"recode",      no_argument,       0, 'r'},
    {"logFile",     required_argument, 0, 'l'},
    {"density",     no_argument,       0, 'd'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-r --recode] [-l --logFile <log file name>] [-d --density] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tsymbol size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tgeneration size\t\t\tDefault: 10\n");
    printf ("\t-r --recode\tenable recoding\n");
    printf ("\t-l --logFile\tlog file name\t\t\tDefault: log.dump\n");
    printf ("\t-d --density\tenable sparse coding\n");
    printf ("\t-h --help\tthis help documetation\n");
}

void print_nc_config (kodo_rlnc::pure_recoder* recoder,
                      bool recode_enable)
{
    // print configuration
    printf ("---------NC configuration---------\n");
    printf ("recoder symbol size:\t\t%u\n", recoder->symbol_size());
    printf ("recoder coeff vector size:\t%u\n", recoder->coefficient_vector_size());
    printf ("recoder payload size:\t\t%u\n", recoder->max_payload_size());
    printf ("recoding enable:\t\t%s\n", recode_enable ? "true" : "false");
    printf ("---------NC configuration---------\n");
}

int write_measurement_log (char* log_file_name,
                           kodo_rlnc::pure_recoder* recoder,
                           bool recode_enable,
                           uint16_t rx_packet_count,
                           bool sparse_enable)
{
    FILE* fp;
    fp = fopen (log_file_name, "a+");
    if (fp == NULL)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, log_file_name, strerror (errno));
        return -1;
    }
    if (sparse_enable == false)
        fprintf(fp, "{\"type\": \"block_full\", \"gen_size\": %u, \"recode\": %d, \"fwd_num\": %u },\n",
            recoder->symbols(),
            recode_enable ? 1 : 0,
            rx_packet_count);
    else
        fprintf(fp, "{\"type\": \"block_sparse\", \"gen_size\": %u, \"recode\": %d, \"fwd_num\": %u },\n",
            recoder->symbols(),
            recode_enable ? 1 : 0,
            rx_packet_count);
    fclose(fp);
    return 0;
}

int main(int argc, char *argv[])
{
    char* serial_port = (char*)USB_DEVICE;
    char* log_file_name = (char*)LOG_FILE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;
    bool recode_enable = false;
    bool sparse_enable = false;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    while ((opt = getopt_long (argc, argv, "p:s:g:rl:dh", long_options, &option_index)) != -1)
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
            case 'l':
                log_file_name = optarg;
                break;
            case 'd':
                sparse_enable = true;
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

    uint32_t inter_frame_interval = 50000; // inter frame interval in us
    int ret;
    int rx_num = 0;
    uint16_t rx_packet_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);

    uint8_t systematic_packet_coeff[4][4] = {
                                            {1, 0, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 0, 1, 0},
                                            {0, 0, 0, 1}
                                            };

    // time related variable definition
    clock_t rx_timeout_start = 0;
    uint32_t rx_timeout = 1100; // ms, hard coded

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

    uint8_t packet[recoder.symbol_size() +
                   recoder.coefficient_vector_size() +
                   IPHC_TOTAL_SIZE +
                   UDPHC_TOTAL_SIZE];
    memset (packet, 0, sizeof packet);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);
    uint32_t tx_packet_length = 0;

    print_nc_config (&recoder, recode_enable);

    rx_timeout_start = clock();
    // relay operations
    while (true)
    {
        if ((clock() - rx_timeout_start) * 1000 / CLOCKS_PER_SEC > rx_timeout)
            break;
        // receive a packet
        rx_num = read_serial_port (fd, extract_buf, &rx_packet_count, false);
        if (rx_num == 0)
            continue;
        else if (rx_num == -1)
        {
            fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
            break;
        }

        if (recode_enable == true)
        {
            printf ("recode a symbol\n");
            if ((unsigned)rx_num == IPHC_TOTAL_SIZE +
                                     UDPHC_TOTAL_SIZE +
                                     sizeof (uint8_t) +
                                     recoder.symbol_size())
            {
                // receive a systematic packet
                // read symbol and coding coefficients into the recoder
                recoder.consume_symbol (extract_buf + sizeof (uint8_t) +
                                        IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                                        systematic_packet_coeff[*(extract_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE)]);
            }
            else if ((unsigned)rx_num == IPHC_TOTAL_SIZE +
                                     UDPHC_TOTAL_SIZE +
                                     recoder.coefficient_vector_size() +
                                     recoder.symbol_size())
            {
                // receive a coded packet
                // read symbol and coding coefficients into the recoder
                recoder.consume_symbol (extract_buf + recoder.coefficient_vector_size() +
                                        IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                                        extract_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE);
            }

            // generate recoding coefficients
            recoder.recoder_generate (recoder_coefficients);
            // write an encoded symbol based on the recoding coefficients
            recoder.recoder_produce_symbol (recoder_symbol,
                                            recoder_symbol_coefficients,
                                            recoder_coefficients);
            // construct packet
            set_ip_header (packet);
            set_udp_header (packet + IPHC_TOTAL_SIZE);
            memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                    recoder_symbol_coefficients,
                    sizeof recoder_symbol_coefficients);
            memcpy (packet + sizeof recoder_symbol_coefficients +
                    IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                    recoder_symbol,
                    sizeof recoder_symbol);
            tx_packet_length = IPHC_TOTAL_SIZE +
                               UDPHC_TOTAL_SIZE +
                               sizeof recoder_symbol_coefficients +
                               sizeof recoder_symbol;
            memset (extract_buf, 0, sizeof extract_buf);
        }
        else
        {
            // construct payload
            memcpy (packet, extract_buf, rx_num);
            tx_packet_length = rx_num;
        }

        // forward packet
        // fragmentation
        if (need_fragmentation (rx_num) == true)
        {
            printf ("lowpan fragmentation needed\n");
            do_fragmentation (tx_packet, packet, tx_packet_length);
            for (uint8_t j = 0; j < get_fragment_num(); j++)
            {
                ret = write_serial_port (fd, tx_packet[j].packet, tx_packet[j].length);
                if (ret < 0)
                    return -1;
                printf ("forward a frame\n");
                usleep (inter_frame_interval);
            }
        }
        else
        {
            generate_normal_packet (&tx_packet[0], packet, tx_packet_length);
            ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
            if (ret < 0)
                return -1;
            printf ("forward a packet\n");
        }
    } // end of while
    printf ("packet total forward: %u\n", rx_packet_count);
    // write log to json file
    ret = write_measurement_log (log_file_name,
                                 &recoder,
                                 recode_enable,
                                 rx_packet_count,
                                 sparse_enable);
    if (ret < 0)
        return -1;
    return 0;
}
