#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"
#include "lowpan.h"
#include "reassemble.h"
#include "utils.h"

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
        printf ("need serial fragmentation\n");
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

uint16_t read_serial_port (int fd, uint8_t* extract_buf)
{
    // parameter definitions
    int rx_num = 0;
    uint8_t rx_buf[128];
    memset (rx_buf, 0, sizeof rx_buf);
    bool first_frame = true;
    bool first_frame_tail_exist = false;
    bool non_frag_frame_tail_exist = false;
    bool read_complete = false;
    uint8_t next_frame_size = 0;

    init_reassembler ();

    while (read_complete == false)
    {
        // read from serial port
        if (first_frame == true)
        {
            rx_num = read (fd, rx_buf, 128);
            if (rx_num > 0)
                printf ("receive %d bytes\n", rx_num);
            else if (rx_num == -1)
            {
                fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
                break;
            }
            else // no data received
                return 0;
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
                is_first_fragment (rx_buf) == true &&
                rx_num > FIRST_FRAG_DATA_OFFSET) ||
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
            if (*rx_buf != rx_num)
            {
                first_frame = false;
                non_frag_frame_tail_exist = true;
                next_frame_size = *rx_buf - rx_num;
                continue;
            }
            printf ("receive a packet\n");
            memcpy (extract_buf,
                    rx_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                    rx_num - IPHC_TOTAL_SIZE - UDPHC_TOTAL_SIZE);
            return (uint16_t)(rx_num - IPHC_TOTAL_SIZE - UDPHC_TOTAL_SIZE);
        }
    } // end of while
    return 0;
}
