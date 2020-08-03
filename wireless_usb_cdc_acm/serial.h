/**
 * author: Xia, Funing
 */

#ifndef SERIAL_H_INCLUDED
#define SERIAL_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    char buf_0[64];
    char buf_1[64];
    uint8_t buf_0_size;
    uint8_t buf_1_size;
} tx_buf_t;

bool m_serial_frag_flag;

bool need_serial_fragmentation (int length);

tx_buf_t* serial_fragmentation (char* data, int length);

tx_buf_t* get_serial_fragmentation_buf (void);

#endif /* SERIAL_H_INCLUDED */
