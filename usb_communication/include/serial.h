#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

typedef struct
{
    char buf_0[64];
    char buf_1[64];
    uint8_t buf_0_size;
    uint8_t buf_1_size;
} tx_buf_t;

int open_serial_port (char* port, int speed, int parity);

int set_interface_attributes (int fd, int speed, int parity);

void set_blocking (int fd, int should_block);

int write_serial_port (int fd, uint8_t* data, int length);

bool need_serial_fragmentation (int length);

tx_buf_t* serial_fragmentation (uint8_t* data, int length);

uint16_t read_serial_port (int fd, uint8_t* extract_buf, uint16_t* rx_frame_count);
#endif /* SERIAL_H */
