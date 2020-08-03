/**
 * author: Xia, Funing
 */

#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "serial.h"

// variable definitions
static tx_buf_t m_tx_buf;

bool need_serial_fragmentation (int length)
{
    return (length > 64);
}

tx_buf_t* serial_fragmentation (char* data, int length)
{
    memset (&m_tx_buf, 0, sizeof m_tx_buf);
    // copy fragments
    memcpy (&m_tx_buf.buf_0[0], data, 64);
    memcpy (&m_tx_buf.buf_1[0], data + 64, length - 64);
    // set fragment size
    m_tx_buf.buf_0_size = 64;
    m_tx_buf.buf_1_size = length - 64;
    return &m_tx_buf;
}

tx_buf_t* get_serial_fragmentation_buf (void)
{
    return &m_tx_buf;
}
