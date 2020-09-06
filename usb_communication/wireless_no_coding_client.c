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

#define USB_DEVICE  "/dev/ttyACM0"
#define LOG_FILE    "log.dump"
#define MAX_SIZE    128

static struct option long_options[] =
{
    {"port",        required_argument, 0, 'p'},
    {"symbolSize",  required_argument, 0, 's'},
    {"genSize",     required_argument, 0, 'g'},
    {"logFile",     required_argument, 0, 'l'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-l --logFile <log file name>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tpayload size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tnumber of packets to be sent\t\t\tDefault: 10\n");
    printf ("\t-l --logFile\tlog file name\t\t\tDefault: log.dump\n");
    printf ("\t-h --help\tthis help documetation\n");
}

int write_measurement_log (char* log_file_name,
                           uint16_t tx_packet_count,
                           uint32_t symbol_size,
                           uint32_t generation_size)
{
    FILE* fp;
    float loss_rate = 0;
    fp = fopen (log_file_name, "a+");
    if (fp == NULL)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, log_file_name, strerror (errno));
        return -1;
    }
    loss_rate = (float)(tx_packet_count - 1) / tx_packet_count;

    fprintf(fp, "{\"type\": \"no_coding\", \"data_size\": %u, \"loss_rate\": %.2f, \"tx_num\": %u },\n",
            symbol_size * generation_size,
            loss_rate,
            tx_packet_count);
    fclose(fp);
    return 0;
}


int main(int argc, char *argv[])
{
    char* serial_port = (char*)USB_DEVICE;
    char* log_file_name = (char*)LOG_FILE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    while ((opt = getopt_long (argc, argv, "p:s:g:l:h", long_options, &option_index)) != -1)
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
    uint8_t rx_buf[MAX_SIZE];
    memset (rx_buf, 0, sizeof rx_buf);
    uint16_t tx_packet_count = 0;
    uint16_t tx_frame_count = 0;
    uint16_t ack_rx_num = 0;
    uint16_t data_offset = 0;
    uint16_t ack_timeout = 100; // ack timeout in ms
    bool tx_success = false;

    // set buffers
    uint8_t data_in[symbol_size * generation_size];
    // fill source data buffer with specific values
    memset (data_in, 'T', sizeof (data_in));

    // set ack packet buffer
    uint8_t ack_packet[MAX_SIZE];
    memset (ack_packet, 0, sizeof ack_packet);

    // set data packet buffer
    uint32_t packet_length = symbol_size +
                             IPHC_TOTAL_SIZE +
                             UDPHC_TOTAL_SIZE;
    uint8_t packet[packet_length];
    memset (packet, 0, sizeof packet);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    // client operations
    while (ack_rx_num < generation_size)
    {
        // reset flag
        tx_success = false;
        // construct packet
        set_ip_header (packet);
        set_udp_header (packet + IPHC_TOTAL_SIZE);
        memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                data_in + data_offset,
                symbol_size);
        // fragmentation
        if (need_fragmentation (packet_length) == true)
            do_fragmentation (tx_packet, packet, packet_length);
        else
            generate_normal_packet (&tx_packet[0], packet, packet_length);

        while (tx_success == false)
        {
            // send a packet
            if (need_fragmentation (packet_length) == true)
            {
                for (uint8_t j = 0; j < get_fragment_num(); j++)
                {
                    ret = write_serial_port (fd, tx_packet[j].packet, tx_packet[j].length);
                    if (ret < 0)
                        return -1;
                    tx_frame_count++;
                    printf ("send a frame\n");
                }
                tx_packet_count++;
            }
            else
            {
                ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
                if (ret < 0)
                    return -1;
                printf ("send a packet\n");
                tx_frame_count++;
                tx_packet_count++;
            }

            clock_t timeout_start = clock();
            while ((clock() - timeout_start) * 1000 / CLOCKS_PER_SEC < ack_timeout)
            {
                // check for ack from server
                rx_num = read_serial_port (fd, rx_buf, NULL);
                if (rx_num > 0 &&
                    strcmp ((const char*)(rx_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE),
                            SERVER_ACK) == 0)
                {
                    printf ("receive ACK from server\n");
                    data_offset += symbol_size;
                    ack_rx_num++;
                    tx_success = true;
                    break;
                }
                else if (rx_num == -1)
                {
                    fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
                    break;
                }
            }
            printf ("ack timeout!\n");
        } // end of while (tx_success)
        // reset packet buffer
        memset (packet, 0, sizeof packet);
    } // end of while (ack_rx_num)
    printf ("packet total send: %u\n", tx_packet_count);
    printf ("frame total send: %u\n", tx_frame_count);
    // send ack to relay node 5 times
    for (uint8_t i = 0; i < 3; i++)
    {
        uint8_t ack_packet_length = generate_ack_packet (ack_packet, (uint8_t*)CLIENT_ACK);
        generate_normal_packet (&tx_packet[0], ack_packet, ack_packet_length);
        ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
        if (ret < 0)
            return -1;
    }

    // write log to json file
    write_measurement_log (log_file_name,
                           tx_packet_count,
                           symbol_size,
                           generation_size);

    return 0;
}
