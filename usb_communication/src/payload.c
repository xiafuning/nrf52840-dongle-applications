#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "payload.h"

/*
   payload structure
   +-----------------+-----------+------+
   | sequence number | timestamp | data |
   +-----------------+-----------+------+
         4 bytes        8 bytes
*/

/**
 * @brief function for generating random payload
 */
uint8_t generate_random_payload (char* payload, uint32_t length, uint32_t seq_number)
{
    // Seed random number generator to produce different results every time
    srand (seq_number);
    
    set_seq_number (payload, seq_number);
    set_timestamp (payload + sizeof (uint32_t));

    for (uint32_t i = 0; i < length; ++i)
    {
        *(payload + 4 + 8 + i) = alphanum[rand () % (sizeof (alphanum) - 1)];
    }
    *(payload + 4 + 8 + length) = '\n';
    return 4 + 8 + length;
}

/**
 * @brief function for setting sequence number
 */
void set_seq_number (char* payload, uint32_t seq_number)
{
    *((uint32_t*)payload) = seq_number;
}

/**
 * @brief function for setting timestamp
 */
int set_timestamp (char* payload)
{
    struct timeval ts;
    int ret;
    ret = gettimeofday (&ts, NULL);
    if (ret < 0)
    {
        fprintf (stderr, "error %d gettimeofday: %s\n", errno, strerror (errno));
        return -1;
    }
    *((long*)payload) = 1000000 * ts.tv_sec + ts.tv_usec;
    return 0;
}

/**
 * @brief function for getting sequence number
 */
uint32_t get_seq_number (char* payload)
{
    uint32_t seq;
    seq = *((uint32_t*)payload);
    return seq;
}

/**
 * @brief function for getting timestamp
 */
long get_timestamp (char* payload)
{
    long timestamp;
    timestamp = *((long*)payload);
    return timestamp;
}

/**
 * @brief function for printing payload
 */
void print_payload (uint8_t* payload, uint32_t length)
{
    printf ("payload: ");
    for (uint32_t i = 0; i < length; ++i)
        printf ("%c", *(payload + i));
    printf ("\n");
}
