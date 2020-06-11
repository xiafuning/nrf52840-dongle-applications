#ifndef PAYLOAD_H
#define PAYLOAD_H

static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

int generate_random_payload (char* payload, uint32_t length, uint32_t seq_number);

void set_seq_number (char* payload, uint32_t seq_number);

int set_timestamp (char* payload);

uint32_t get_seq_number (char* payload);

long get_timestamp (char* payload);

void print_payload (char* payload, uint32_t length);

#endif /* PAYLOAD_H */
