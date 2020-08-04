#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"

// variable definitions
static tx_buf_t m_tx_buf;

int open_serial_port (char* port, int speed, int parity)
{
    int fd = open (port, O_RDWR | O_NOCTTY | O_SYNC);
    fcntl(fd, F_SETFL, 0); // set blocking
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
    tty.c_cc[VTIME] = 10;                           // 0.1 seconds read timeout

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
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        fprintf (stderr, "error %d setting term attributes\n", errno);
}

int write_serial_port (int fd, char* data, int length)
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

tx_buf_t* serial_fragmentation (char* data, int length)
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
