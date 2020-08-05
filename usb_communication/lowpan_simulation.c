#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>

#include "serial.h"
#include "payload.h"
#include "lowpan.h"
#include "reassemble.h"

#define LOG_ON false

static struct option long_options[] =
{
    {"length", required_argument, 0, 'l'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-l --length <payload length>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-l --length\tpayload length\n");
    printf ("\t-h --help\tthis help documetation\n");
}

int main(int argc, char *argv[])
{
    uint16_t payload_length = 0;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    if (argc == 1)
    {
        usage ();
        return 0;
    }

    while ((opt = getopt_long (argc, argv, "l:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'l':
                payload_length = atoi(optarg);
                break;
            case 'h':
                usage ();
                return 0;
                break;
            default:
                usage ();
                break;
        }
    }
    
    // Seed random number generator to produce different results every time
    srand (static_cast<uint32_t> (time (0)));

    // lowpan test
    uint8_t payload[payload_length];
    memset (payload, 'a', sizeof payload);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    // fragmentation
    if (need_fragmentation (payload_length) == false)
    {
        printf ("no fragmentation needed\n");
        generate_normal_packet (&tx_packet[0], payload, payload_length);
        print_payload (tx_packet[0].packet, tx_packet[0].length);
    }
    else
    {
        printf ("fragmentation needed\n");
        do_fragmentation (tx_packet, payload, payload_length);
        //printf ("number of fragments: %u\n", get_fragment_num());
        //printf ("tail size: %u\n", get_tail_size());
    }

    if (LOG_ON == true)
        for (uint8_t i = 0; i < get_fragment_num(); i++)
        {
            printf ("----------------------\nfragment %u\n", i);
            //print_payload (tx_packet[i].packet, tx_packet[i].length);
            printf ("packet size: %u\n", tx_packet[i].length);
            printf ("datagram size: %u\n", get_datagram_size (tx_packet[i].packet));
            printf ("datagram tag: %u\n", get_datagram_tag (tx_packet[i].packet + 2));
            if (i != 0)
                printf ("datagram offset: %u\n", get_datagram_offset (tx_packet[i].packet + 4));
        }

    // reassemble
    init_reassembler ();
    for (uint8_t i = 0; i < get_fragment_num(); i++)
    {
        if (need_reassemble (tx_packet[i].packet))
        {
            printf("read a frame\n");
            read_frame(tx_packet[i].packet, tx_packet[i].length);
        }
        if (is_reassemble_complete ())
            printf ("reassemble complete!\n");
    }
    return 0;
}
