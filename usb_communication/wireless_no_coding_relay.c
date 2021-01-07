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
    printf ("\t-l --logFile\tlog file name\t\tDefault: log.dump\n");
    printf ("\t-h --help\tthis help documetation\n");
}

int write_measurement_log (char* log_file_name,
                           uint16_t rx_frame_count,
                           uint16_t tx_frame_count,
                           uint16_t fwd_frame_count,
                           uint32_t symbol_size,
                           uint32_t generation_size)
{
    FILE* fp;
    fp = fopen (log_file_name, "a+");
    if (fp == NULL)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, log_file_name, strerror (errno));
        return -1;
    }

    fprintf(fp, "{\"type\": \"no_coding\", \"data_size\": %u, \"rx_num\": %u, \"fwd_num\": %u, \"ack_rx_num\": %u },\n",
            symbol_size * generation_size,
            rx_frame_count,
            tx_frame_count,
            fwd_frame_count);
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

    int ret = 0;
    int rx_num = 0;
    uint8_t rx_buf[MAX_SIZE];
    memset (rx_buf, 0, sizeof rx_buf);
    uint16_t rx_frame_count = 0;
    uint16_t tx_frame_count = 0;
    uint16_t fwd_frame_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);

    // time related variable definition
    clock_t rx_timeout_start = 0;
    uint32_t rx_timeout = 1500; // ms, hard coded
    clock_t ack_timeout_start = 0;
    uint32_t ack_timeout = 50; // ms, hard coded

    // set buffers
    uint8_t data_out[symbol_size * generation_size];
    memset (data_out, 0, sizeof data_out);

    // set ack message buffer
    virtual_packet_t ack_buf;
    uint8_t ack_packet[MAX_SIZE];
    memset (ack_packet, 0, sizeof ack_packet);
    memset (&ack_buf, 0, sizeof ack_buf);
    // construct ack packet
    uint8_t ack_packet_length = generate_ack_packet (ack_packet, (uint8_t*)RELAY_ACK);

    // create a forwarder
    lowpan_forwarder_t forwarder;
    init_forwarder (&forwarder);

    rx_timeout_start = clock();
    // relay operations
    while ((clock() - rx_timeout_start) * 1000 / CLOCKS_PER_SEC < rx_timeout)
    {
        // receive a packet
        rx_num = read_serial_port (fd, extract_buf, NULL, true);
        if (rx_num > 0)
        {
            // receive an ack means a frame is successfully forwarded
            // reset forwarder
            if (is_ack_packet (extract_buf) == true)
            {
                printf ("[relay] forward a frame\n");
                forwarder.idle = true;
                forwarder.read_index++;
                if (forwarder.read_index == FORWARDER_QUEUE_LENGTH)
                    forwarder.read_index = 0;
                forwarder.frame_tries = 0;
                fwd_frame_count++;
            }
            // receive a data frame, need to be forwarded
            else
            {
                // send ack
                printf ("[relay] send ACK\n");
                ret = write_serial_port (fd, ack_packet, ack_packet_length);
                if (ret == -1)
                    return 0;
                rx_frame_count++;
                // insert frame in forwarding queue
                memcpy (forwarder.queue[forwarder.write_index].packet,
                        extract_buf,
                        rx_num);
                forwarder.queue[forwarder.write_index].length = rx_num;
                forwarder.write_index++;
                if (forwarder.write_index == FORWARDER_QUEUE_LENGTH)
                    forwarder.write_index = 0;
            }
        }
        // no ongoing forwarding process
        if (forwarder.idle == true &&
            is_forwarder_empty (&forwarder) == false)
        {
            forwarder_send (fd, &forwarder);
            ack_timeout_start = clock();
            tx_frame_count++;
        }
        // a frame is in forwarding process
        else
        {
            if (is_forwarder_empty (&forwarder) == true ||
                (clock() - ack_timeout_start) * 1000 / CLOCKS_PER_SEC < ack_timeout)
                continue;
            else if (forwarder.frame_tries >= MAC_MAX_RETRIES + 1)
            {
                forwarder.frame_tries = 0;
                forwarder.read_index++;
                if (forwarder.read_index == FORWARDER_QUEUE_LENGTH)
                    forwarder.read_index = 0;
                if (is_forwarder_empty (&forwarder) == true)
                    continue;
                else
                {
                    forwarder_send (fd, &forwarder);
                    ack_timeout_start = clock();
                    tx_frame_count++;
                }
            }
            else
            {
                forwarder_send (fd, &forwarder);
                ack_timeout_start = clock();
                tx_frame_count++;
            }
        }
    } // end of while

    // write log to json file
    write_measurement_log (log_file_name,
                           rx_frame_count,
                           tx_frame_count,
                           fwd_frame_count,
                           symbol_size,
                           generation_size);

    printf ("[relay] frame total receive: %u\n", rx_frame_count);
    printf ("[relay] frame total send: %u\n", tx_frame_count);
    printf ("[relay] frame total forward: %u\n", fwd_frame_count);

    return 0;
}
