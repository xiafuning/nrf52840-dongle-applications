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
    printf ("\t-s --symbolSize\tpayload size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tnumber of packets to be sent\t\t\tDefault: 10\n");
    printf ("\t-h --help\tthis help documetation\n");
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
    uint16_t rx_packet_count = 0;
    uint16_t rx_frame_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);
    uint16_t last_udp_checksum = 0;
    uint16_t data_offset = 0;

    // set buffers
    uint8_t data_out[symbol_size * generation_size];
    memset (data_out, 0, sizeof data_out);

    // set ack message buffer
    virtual_packet_t ack_buf;
    uint8_t ack_packet[MAX_SIZE];
    memset (ack_packet, 0, sizeof ack_packet);
    memset (&ack_buf, 0, sizeof ack_buf);
    // construct ack packet
    uint8_t ack_packet_length = generate_ack_packet (ack_packet, (uint8_t*)SERVER_ACK);
    generate_normal_packet (&ack_buf, ack_packet, ack_packet_length);

    // server operations
    while (rx_packet_count < generation_size)
    {
        // receive a packet
        rx_num = read_serial_port (fd, extract_buf, &rx_frame_count);
        if (rx_num == 0)
            continue;
        //print_payload (extract_buf, rx_num);
        if (get_udp_checksum (get_udp_header (extract_buf) + 5) != last_udp_checksum)
        {
            // save udp checksum
            last_udp_checksum = get_udp_checksum (get_udp_header (extract_buf) + 5);
            // copy payload
            memcpy (data_out + data_offset,
                    extract_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                    rx_num - IPHC_TOTAL_SIZE - UDPHC_TOTAL_SIZE);
            // send ACK message
            ret = write_serial_port (fd, ack_buf.packet, ack_buf.length);
            if (ret < 0)
                return -1;
            memset (extract_buf, 0, sizeof extract_buf);
            data_offset += symbol_size;
            rx_packet_count++;
        }
        else
        {
            // if receive the same packet again
            // send ACK message again
            ret = write_serial_port (fd, ack_buf.packet, ack_buf.length);
            if (ret < 0)
                return -1;
        }
    } // end of while
    printf ("receive complete!\n");
    //print_payload (data_out, sizeof data_out);
    printf ("packet total receive: %u\n", rx_packet_count);
    printf ("frame total receive: %u\n", rx_frame_count);
    return 0;
}
