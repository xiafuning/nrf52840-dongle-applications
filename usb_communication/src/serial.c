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

uint16_t read_serial_port (int fd, uint8_t* extract_buf, uint16_t* rx_frame_count)
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
    int ret = 0;
    uint8_t ack_packet[64];
    // construct ack packet
    uint8_t ack_packet_length = generate_ack_packet (ack_packet, (uint8_t*)SERVER_ACK);

    // time related variable definition
    clock_t frame_rx_timeout_start = 0;
    uint32_t next_frame_rx_timeout = 70; // ms, hard coded
    uint32_t packet_rx_timeout = 1500; // ms, hard coded
    clock_t packet_rx_timeout_start = 0;

    // last fragment handle variables
    bool last_fragment_handle = false;
    int last_fragment_handle_drop_size = 0;

    init_reassembler();

    packet_rx_timeout_start = clock();
    while (read_complete == false)
    {
        if ((clock() - packet_rx_timeout_start) * 1000 / CLOCKS_PER_SEC > packet_rx_timeout)
            return 0;
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
            frame_rx_timeout_start = clock();
            do
            {
                rx_num += read (fd, rx_buf + rx_num, 1);
                if ((clock() - frame_rx_timeout_start) * 1000 / CLOCKS_PER_SEC > next_frame_rx_timeout)
                    return 0;
            }
            while (rx_num < *rx_buf);
            printf ("receive %d bytes total\n", rx_num);
            non_frag_frame_tail_exist = false;
            first_frame = true;
        }
        else if (last_fragment_handle == true)
        {
            printf ("drop useless data\n");
            last_fragment_handle = false;
            rx_num = 0;
            frame_rx_timeout_start = clock();
            do
            {
                rx_num += read (fd, rx_buf + rx_num, 1);
                if ((clock() - frame_rx_timeout_start) * 1000 / CLOCKS_PER_SEC > next_frame_rx_timeout)
                    return 0;
            }
            while (rx_num < last_fragment_handle_drop_size);
            continue;
        }
        else
        {
            rx_num = 0;
            frame_rx_timeout_start = clock();
            do
            {
                rx_num += read (fd, rx_buf + rx_num, 1);
                if ((clock() - frame_rx_timeout_start) * 1000 / CLOCKS_PER_SEC > next_frame_rx_timeout)
                    break;
            }
            while (rx_num < next_frame_size);
            // handle the case that first frame
            // is incomplete in the first rx
            if (rx_num > 0)
                printf ("receive %d bytes\n", rx_num);
            if (first_frame_tail_exist == true)
            {
                next_frame_size = copy_frame_tail (rx_buf, rx_num);
                first_frame_tail_exist = false;
                continue;
            }
        }

        if (rx_num == 0)
            continue;
        else
            print_payload (rx_buf, rx_num);

        // check if frame is correctly formatted
        if (is_frame_format_correct (rx_buf) == false)
        {
            // if not, reset reassembler
            init_reassembler ();
            first_frame = true;
            first_frame_tail_exist = false;
            non_frag_frame_tail_exist = false;
            next_frame_size = 0;
            continue;
        }

        // process received frame
        if (need_reassemble (rx_buf)) // fragmented packet
        {
            // send ack
            ret = write_serial_port (fd, ack_packet, ack_packet_length);
            if (ret == -1)
                return 0;
            // update frame counter
            if (rx_frame_count != NULL)
                (*rx_frame_count)++;
            // receive first frame of a packet
            // and make sure part of the payload is
            // included to avoid segmentation fault
            if (is_first_fragment (rx_buf) == true &&
                rx_num > FIRST_FRAG_DATA_OFFSET)
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
                     is_new_packet (rx_buf) == false &&
                     is_new_fragment (rx_buf) == true)
                next_frame_size = read_frame (rx_buf, rx_num);
            // handle last and second last fragment conflict
            // contain magic number here!
            else if (is_reassembler_running () == true &&
                     is_new_packet (rx_buf) == false &&
                     is_new_fragment (rx_buf) == false &&
                     get_datagram_offset (rx_buf + 4) == 160 && // 0x14 << 3 = 160
                     rx_num == 23)
            {
                last_fragment_handle = true;
                // size of second last fragment payload: 93
                last_fragment_handle_drop_size = 93 - 23;
                continue;
            }
            // other cases
            else
            {
                memset (rx_buf, 0, sizeof rx_buf);
                continue;
            }
            // check if next frame size is valid
            if (next_frame_size > sizeof rx_buf)
            {
                // if not, reset reassembler
                printf ("incorrect next frame size\n");
                init_reassembler ();
                first_frame = true;
                first_frame_tail_exist = false;
                non_frag_frame_tail_exist = false;
                next_frame_size = 0;
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
            if (rx_frame_count != NULL)
                (*rx_frame_count)++;
            memcpy (extract_buf, rx_buf, rx_num);
            return (uint16_t)rx_num;
        }
    } // end of while
    return 0;
}

bool wait_ack (int fd, uint16_t ack_timeout, char* ack_message)
{
    uint8_t rx_buf[64];
    int rx_num = 0;
    memset (rx_buf, 0, sizeof rx_buf);
    // wait for ack
    clock_t timeout_start = clock();
    while ((clock() - timeout_start) * 1000 / CLOCKS_PER_SEC < ack_timeout)
    {
        // check for ack from server
        rx_num = read_serial_port (fd, rx_buf, NULL);
        if (rx_num > 0 &&
            strcmp ((const char*)(rx_buf + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE),
                    ack_message) == 0)
            return true;
        else if (rx_num == -1)
        {
            fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
            break;
        }
    }
    return false;
}
