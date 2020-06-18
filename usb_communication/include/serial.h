#ifndef SERIAL_H
#define SERIAL_H

int open_serial_port (char* port, int speed, int parity);

int set_interface_attributes (int fd, int speed, int parity);

void set_blocking (int fd, int should_block);

int write_serial_port (int fd, char* data, int length);

#endif /* SERIAL_H */
