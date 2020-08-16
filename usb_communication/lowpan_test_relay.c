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
    {"help", no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-h --help\tthis help documetation\n");
}

void forward_packet (int fd, uint8_t* packet, uint8_t length)
{
    write_serial_port (fd, packet, length);
    printf ("forward a packet, %u bytes\n", length);
    print_payload (packet, length);
}

/**
 * @brief check if a frame is correctly formatted
 */
bool frame_format_check (uint8_t* frame,
                         bool idle,
                         uint16_t datagram_size,
                         uint16_t datagram_tag)
{
    // check for non-fragmented packet
    if ((*(frame + 1)) == 'I' && (*(frame + 2)) == 'I' && (*(frame + 3)) == 'U')
        return true;

    // check for fragmented packet
    if (idle == false && ((*frame) >> 5 != 0x07 ||
        get_datagram_size (frame) != datagram_size ||
        get_datagram_tag (frame + 2) != datagram_tag))
        return false;
    else if (idle == true && (*frame) >> 5 != 0x06)
        return false;
   else
        return true;
}

int main(int argc, char *argv[])
{
    char rx_buf[MAX_SIZE];
    char* serial_port = (char*)USB_DEVICE;

    memset (rx_buf, 0, sizeof rx_buf);

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    if (argc == 1)
    {
        usage ();
        return 0;
    }

    while ((opt = getopt_long (argc, argv, "p:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                serial_port = optarg;
                break;
           case 'h':
                usage ();
                return 0;
                break;
            default:
                usage ();
                return 0;
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

    // lowpan test relay
    int rx_num = 0;
    uint8_t rx_count = 0;
    uint8_t extract_buf[MAX_PACKET_SIZE];
    memset (extract_buf, 0, sizeof extract_buf);
    bool first_frame = true;
    bool first_frame_tail_exist = false;
    bool non_frag_frame_tail_exist = false;
    uint8_t next_frame_size = 0;

    uint16_t datagram_size = 0;
    uint16_t datagram_tag = 0;
    uint8_t fragment_num = 0;
    uint8_t current_frame = 0;
    uint8_t rx_num_order[MAX_FRAG_NUM];
    bool idle = true;
    memset (rx_num_order, 0, sizeof rx_num_order);

    while (true)
    {
        // reassemble
        init_reassembler ();
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
            do
                rx_num += read (fd, rx_buf + rx_num, 1);
            while (rx_num < next_frame_size);
            // handle the case that first frame
            // is incomplete in the first rx
            printf ("receive %d bytes\n", rx_num);
            if (first_frame_tail_exist == true)
            {
                first_frame_tail_exist = false;
                // forward completed first frame
                forward_packet (fd, rx_buf, rx_num);
                current_frame++;
                next_frame_size = rx_num_order[current_frame];
                rx_num = 0;
                continue;
            }
        }
        // check if frame is correctly formatted
        if (frame_format_check (rx_buf, idle, datagram_size, datagram_tag) == false)
        {
            // if not, reset parameters
            printf ("incorrect format\n");
            print_payload (rx_buf, rx_num);
            idle = true;
            first_frame = true;
            first_frame_tail_exist = false;
            non_frag_frame_tail_exist = false;
            next_frame_size = 0;
            datagram_size = 0;
            datagram_tag = 0;
            fragment_num = 0;
            memset (rx_num_order, 0, sizeof rx_num_order);
            continue;
        }

        // process received packet
        if (need_reassemble (rx_buf)) // fragmented packet
        {
            if (// last packet is reassembled and receive
                // first frame of a new packet
                (idle == true && is_first_fragment (rx_buf) == true) ||
                // last packet is in reassembling and receive
                // first frame of a new packet (at least one
                // fragment of last packet is lost)
                (idle == false &&
                 datagram_tag != get_datagram_tag (rx_buf + 2) &&
                 is_first_fragment (rx_buf) == true) &&
                 rx_num > FIRST_FRAG_DATA_OFFSET)
            {
                idle = false;
                datagram_size = get_datagram_size (rx_buf);
                datagram_tag = get_datagram_tag (rx_buf + 2);
                fragment_num = calculate_fragment_num (datagram_size);
                calculate_rx_num_order (rx_num_order, fragment_num, datagram_size);
                current_frame = 0;
                if (first_frame == true)
                    first_frame = false;
                if (rx_num < rx_num_order[0])
                {
                    first_frame_tail_exist = true;
                    next_frame_size = rx_num_order[current_frame];
                    continue;
                }
                // forward packet
                forward_packet (fd, rx_buf, rx_num);
                // shift current frame position and set new next frame size
                current_frame++;
                next_frame_size = rx_num_order[current_frame];
                rx_num = 0;
            }
            // receive a fragment of a known packet
            else if (idle == false &&
                     datagram_tag == get_datagram_tag (rx_buf + 2))
            {
                // forward packet
                forward_packet (fd, rx_buf, rx_num);
                // shift current frame position and set new next frame size
                current_frame++;
                next_frame_size = rx_num_order[current_frame];
                rx_num = 0;
            }
            // other cases
            else
            {
                memset (rx_buf, 0, sizeof rx_buf);
                continue;
            }
            if (current_frame == fragment_num) // complete forwarding a fragmented packet
            {
                printf ("packet %u forwarded!\n", rx_count);
                rx_count++;
                idle = true;
                next_frame_size = 0;
                rx_num = 0;
                first_frame = true;
                first_frame_tail_exist = false;
                datagram_size = 0;
                datagram_tag = 0;
                fragment_num = 0;
                memset (rx_num_order, 0, sizeof rx_num_order);
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
            // forward packet
            forward_packet (fd, rx_buf, rx_num);
            rx_count++;
        }
    } // end of while
    return 0;
}
