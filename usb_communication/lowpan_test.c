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

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128

static struct option long_options[] =
{
    {"port", required_argument, 0, 'p'},
    {"mode", required_argument, 0, 'm'},
    {"length", required_argument, 0, 'l'},
    {"help", no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-m --mode <mode>] [-l --length <payload length>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-m --mode\trunning mode\t\t\tOptions: client/server\n");
    printf ("\t-l --length\tpayload length\n");
    printf ("\t-h --help\tthis help documetation\n");
}

int main(int argc, char *argv[])
{
    char rx_buf[MAX_SIZE];
    bool client = false;
    bool server = false;
    char* serial_port = (char*)USB_DEVICE;
    uint16_t payload_length = 0;

    memset (rx_buf, 0, sizeof rx_buf);

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    if (argc == 1)
    {
        usage ();
        return 0;
    }

    while ((opt = getopt_long (argc, argv, "p:m:l:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                serial_port = optarg;
                break;
            case 'm':
                if (!strcmp (optarg, "client"))
                    client = true;
                else if (!strcmp (optarg, "server"))
                    server = true;
                else
                {
                    fprintf (stderr, "error: unknown mode %s\n", optarg);
                    return -1;
                }
                break;           
            case 'l':
                payload_length = atoi(optarg);
                break;
            case 'h':
                usage ();
                return 0;
                break;
            default:
                usage ();
                break;
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

    // lowpan test
    uint32_t num_packets = 30;
    uint32_t inter_frame_interval = 100000; // inter frame interval in us
    int ret;

    uint8_t payload[payload_length];
    memset (payload, 0, sizeof payload);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    if (client)
    {
        for (uint8_t i = 0; i < num_packets; ++i)
        {
            memset (payload, '0' + i, sizeof payload);
            // fragmentation
            if (need_fragmentation (payload_length) == true)
            {
                printf ("lowpan fragmentation needed\n");
                do_fragmentation (tx_packet, payload, payload_length);
                for (uint8_t j = 0; j < get_fragment_num(); j++)
                {
                    printf ("frame length: %u\n", tx_packet[j].length);
                    print_payload (tx_packet[j].packet, tx_packet[j].length);
                    ret = write_serial_port (fd, tx_packet[j].packet, tx_packet[j].length);
                    if (ret < 0)
                        return -1;
                    printf ("send a frame\n");
                    usleep (inter_frame_interval);
                }
            }
            else
            {
                generate_normal_packet (&tx_packet[0], payload, payload_length);
                ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
                if (ret < 0)
                    return -1;
                printf ("send a frame\n");
                usleep (inter_frame_interval);
            }
        }
    }

    if (server)
    {
        int rx_num = 0;
        uint8_t rx_count = 0;
        uint8_t extract_buf[MAX_PACKET_SIZE];
        memset (extract_buf, 0, sizeof extract_buf);
        bool first_frame = true;
        bool first_frame_tail_exist = false;
        bool non_frag_frame_tail_exist = false;
        uint8_t next_frame_size = 0;

        // reassemble
        init_reassembler ();
        while (rx_count < num_packets)
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
                else if (is_reassembler_running () == true &&
                    is_new_packet (rx_buf) == false)
                    next_frame_size = read_frame (rx_buf, rx_num);
                else
                {
                    memset (rx_buf, 0, sizeof rx_buf);
                    continue;
                }
                if (is_reassemble_complete ())
                {
                    printf ("packet %u reassemble complete!\n", rx_count);
                    extract_packet (extract_buf);
                    print_payload (extract_buf, sizeof extract_buf);
                    rx_count++;
                    next_frame_size = 0;
                    first_frame = true;
                    init_reassembler ();
                    memset (extract_buf, 0, sizeof extract_buf);
                    if (rx_count == num_packets)
                        return 0;
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
                print_payload (rx_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE, rx_num);
                rx_count++;
            }
            // clear rx buffer after processing
            memset (rx_buf, 0, sizeof rx_buf);
        } // end while
    } // end if (server)
    return 0;
}
