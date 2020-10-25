#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "serial.h"
#include "lowpan.h"
#include "reassemble.h"
#include "utils.h"
#include "config.h"


// variable definitions
static tx_buf_t m_tx_buf;

int open_serial_port (char* port, int speed, int parity)
{
    int fd = open (port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, port, strerror (errno));
        return -1;
    }

    set_interface_attributes (fd, speed, parity);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (fd, 0);                       // set no blocking

    return fd;
}

int set_interface_attributes (int fd, int speed, int parity)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        fprintf (stderr, "error %d from tcgetattr\n", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                         // disable break processing
    tty.c_iflag &= ~(INLCR | ICRNL);                // avoid 0x0d to 0x0a problem
    tty.c_lflag &= ~ICANON;                         // disable canonical mode
    tty.c_lflag &= ~ECHO;                           // disable echo
    tty.c_lflag &= ~ISIG;                           // disable interpretation of INTR, QUIT and SUSP
    tty.c_oflag = 0;                                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;                            // read doesn't block
    tty.c_cc[VTIME] = 0;                            // 0.1 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // shut off xon/xoff ctrl
    tty.c_cflag |= (CLOCAL | CREAD);                // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);              // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;                         // clear stop field(only one stop bit)
    tty.c_cflag &= ~CRTSCTS;                        // disable RTS/CTS hardware flow control

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    // save tty settings and check for error
    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        fprintf (stderr, "error %d from tcsetattr\n", errno);
        return -1;
    }
    return 0;
}

void set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        fprintf (stderr, "error %d from tggetattr\n", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        fprintf (stderr, "error %d setting term attributes\n", errno);
}

int write_serial_port (int fd, uint8_t* data, int length)
{
    int ret;
    if (need_serial_fragmentation (length) == true)
    {
        tx_buf_t* tx_buffer;
        //printf ("need serial fragmentation\n");
        tx_buffer = serial_fragmentation (data, length);
        // send first serial fragment
        ret = write (fd, tx_buffer->buf_0, tx_buffer->buf_0_size);
        if (ret < 0)
        {
            fprintf (stderr, "error %d write fail: %s\n", errno, strerror (errno));
            return -1;
        }
        // send second serial fragment
        ret = write (fd, tx_buffer->buf_1, tx_buffer->buf_1_size);
        if (ret < 0)
        {
            fprintf (stderr, "error %d write fail: %s\n", errno, strerror (errno));
            return -1;
        }
        return 0;
    }
    else
    {
        ret = write (fd, data, length);
        if (ret < 0)
        {
            fprintf (stderr, "error %d write fail: %s\n", errno, strerror (errno));
            return -1;
        }
        else
            return 0;
    }
}

bool need_serial_fragmentation (int length)
{
    return (length > 64);
}

tx_buf_t* serial_fragmentation (uint8_t* data, int length)
{
    memset (&m_tx_buf, 0, sizeof m_tx_buf);
    // set serial fragment indicator
    m_tx_buf.buf_0[0] = 1;
    m_tx_buf.buf_1[0] = 2;
    memcpy (&m_tx_buf.buf_0[1], data, 63);
    memcpy (&m_tx_buf.buf_1[1], data + 63, length - 63);
    m_tx_buf.buf_0_size = 64;
    m_tx_buf.buf_1_size = length - 62;
    return &m_tx_buf;
}

uint16_t read_serial_port (int fd, uint8_t* extract_buf, uint16_t* rx_frame_count, bool frame_only)
{
    // parameter definitions
    int rx_num = 0;
    uint8_t rx_buf[128];
    memset (rx_buf, 0, sizeof rx_buf);
    bool read_complete = false;
    int ret = 0;
    uint8_t ack_packet[64];
    // construct ack packet
    uint8_t ack_packet_length = generate_ack_packet (ack_packet, (uint8_t*)SERVER_ACK);

    // time related variable definition
    uint32_t packet_rx_timeout = 1500; // ms, hard coded
    clock_t packet_rx_timeout_start = 0;

    uint8_t current_frame_length = 0;
    uint8_t init_rx_size = 10;
    uint16_t datagram_size = 0;
    uint8_t fragment_num = 0;
    uint8_t rx_num_order[MAX_FRAG_NUM];
    memset (rx_num_order, 0, sizeof rx_num_order);
    uint16_t sum = 0;

    init_reassembler();

    packet_rx_timeout_start = clock();
    while (read_complete == false)
    {
        if ((clock() - packet_rx_timeout_start) * 1000 / CLOCKS_PER_SEC > packet_rx_timeout)
            return 0;
        // read from serial port
        rx_num = read (fd, rx_buf, init_rx_size);
        if (rx_num > 0)
        {
            // fragmented packet
            if (need_reassemble (rx_buf) == true)
            {
                datagram_size = get_datagram_size (rx_buf);
                fragment_num = calculate_fragment_num (datagram_size);
                calculate_rx_num_order (rx_num_order, fragment_num, datagram_size);

                if (is_first_fragment (rx_buf) == true)
                    current_frame_length = rx_num_order[0];
                else
                {
                    for (uint8_t i = 1; i < fragment_num; i++)
                    {
                        sum = 0;
                        for (uint8_t j = 0; j < i; j++)
                            if (j == 0)
                                sum += rx_num_order[j] - FIRST_FRAG_HDR_SIZE;
                            else
                                sum += rx_num_order[j] - OTHER_FRAG_HDR_SIZE;
                        if (sum == get_datagram_offset (rx_buf + 4))
                        {
                            current_frame_length = rx_num_order[i];
                            break;
                        }
                    }
                }
            }
            // non-fragmented/normal packet
            else
                current_frame_length = *rx_buf;
            do
            {
                rx_num += read (fd, rx_buf + rx_num, 1);
            }
            while (rx_num < current_frame_length);
            printf ("receive %d bytes\n", rx_num);
        }
        else if (rx_num == -1)
        {
            fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
            break;
        }
        else if (is_reassembler_running() == true)
            continue;
        else
            return 0;

        if (rx_num == 0)
            continue;
        else
            print_payload (rx_buf, rx_num);

        // update frame counter
        if (rx_frame_count != NULL)
            (*rx_frame_count)++;
        // return the received frame
        if (frame_only == true)
        {
            printf ("receive a frame\n");
            memcpy (extract_buf, rx_buf, rx_num);
            return (uint16_t)rx_num;
        }

        // check if frame is correctly formatted
        if (is_frame_format_correct (rx_buf) == false)
        {
            // if not, reset reassembler
            init_reassembler ();
            continue;
        }

        // process received frame
        if (need_reassemble (rx_buf)) // fragmented packet
        {
            // send ack
            printf ("send ACK\n");
            ret = write_serial_port (fd, ack_packet, ack_packet_length);
            if (ret == -1)
                return 0;
            // receive first frame of a packet
            // and make sure part of the payload is
            // included to avoid segmentation fault
            if (is_first_fragment (rx_buf) == true &&
                rx_num > FIRST_FRAG_DATA_OFFSET)
            {
                start_new_reassemble (rx_buf);
                read_frame (rx_buf, rx_num);
            }
            // receive a fragment of a known packet
            else if (is_reassembler_running () == true &&
                     is_new_packet (rx_buf) == false &&
                     is_new_fragment (rx_buf) == true)
                read_frame (rx_buf, rx_num);
            // other cases
            else
            {
                memset (rx_buf, 0, sizeof rx_buf);
                continue;
            }
            if (is_reassemble_complete () == true)
            {
                printf ("packet reassemble complete!\n");
                // extract reassembled packet
                extract_packet (extract_buf);
                // set read complete flag
                read_complete = true;
                reassembler_t* reassembler = get_reassembler ();
                return reassembler->filled_size;
            }
        }
        else // non-fragmented/normal packet
        {
            printf ("receive a packet\n");
            memcpy (extract_buf, rx_buf, rx_num);
            return (uint16_t)rx_num;
        }
    } // end of while
    return 0;
}

bool wait_ack (int fd, uint16_t ack_timeout)
{
    uint8_t rx_buf[64];
    int rx_num = 0;
    memset (rx_buf, 0, sizeof rx_buf);
    // wait for ack
    clock_t timeout_start = clock();
    while ((clock() - timeout_start) * 1000 / CLOCKS_PER_SEC < ack_timeout)
    {
        // check for ack from server
        rx_num = read_serial_port (fd, rx_buf, NULL, false);
        if (rx_num > 0 &&
            (strcmp ((const char*)(rx_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE), RELAY_ACK) == 0 ||
             strcmp ((const char*)(rx_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE), SERVER_ACK) == 0))
            return true;
        else if (rx_num == -1)
        {
            fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
            break;
        }
    }
    return false;
}
