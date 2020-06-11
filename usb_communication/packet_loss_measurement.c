#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/time.h>

#include "serial.h"
#include "payload.h"

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128

static struct option long_options[] =
{
    {"port", required_argument, 0, 'p'},
    {"mode", required_argument, 0, 'm'},
    {"help", no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-m --mode <mode>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-m --mode\trunning mode\t\t\tOptions: client/server\n");
    printf ("\t-h --help\tthis help documetation\n");
}


int main(int argc, char *argv[])
{
    char str[MAX_SIZE];
    char rx_buf[MAX_SIZE];
    bool client = false;
    bool server = false;
    char* serial_port = (char*)USB_DEVICE;

    memset (str, 0, sizeof str);
    memset (rx_buf, 0, sizeof rx_buf);

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    if (argc == 1)
    {
        usage ();
        return 0;
    }

    while ((opt = getopt_long (argc, argv, "p:m:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                serial_port = optarg;
                break;
            case 'm':
                if (!strcmp (optarg, "client"))
                    client = true;
                else if (!strcmp (optarg, "server"))
                    server = true;
                else
                {
                    fprintf (stderr, "error: unknown mode %s\n", optarg);
                    return -1;
                }
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

    // USB communication
    int fd = open_serial_port (serial_port, B115200, 0);
    if (fd < 0)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, serial_port, strerror (errno));
        return -1;
    }

    uint32_t seq = 0;
    uint32_t num_packets = 100;
    uint32_t payload_length = 4;
    uint32_t rx_counter = 0;
    uint32_t inter_packet_interval = 10000; // inter packet interval in us

    if (client)
    {
        int ret;
        for (uint32_t i = 0; i < num_packets; ++i)
        {
            generate_random_payload (str, payload_length, seq);
            ret = write (fd, str, 4 + 8 + payload_length);
            if (ret < 0)
            {
                fprintf (stderr, "error %d write fail: %s\n", errno, strerror (errno));
                break;
            }
            memset (str, 0, sizeof str);
            printf ("send packet %u\n", seq);
            seq++;
            usleep (inter_packet_interval);
        }
    }

    if (server)
    {
        int rx_num;
        bool first_packet = false;
        bool rx_start = false;

        struct timeval ts_start;
        struct timeval ts_actual;
        
        while (true)
        {
            if (first_packet && rx_start == false)
            {
                gettimeofday (&ts_start, NULL);
                rx_start = true;
            }
            rx_num = read (fd, rx_buf, MAX_SIZE);
            if (rx_num > 0)
            {
                printf ("receive %d bytes, sequence number: %" PRIu32 "\n", rx_num, get_seq_number (rx_buf));
                //print_payload (rx_buf + 4 + 8, payload_length);
                rx_counter++;
                if (first_packet == false)
                    first_packet = true;
                // clear rx buffer after processing
                memset (rx_buf, 0, sizeof rx_buf);
            }
            else if (rx_num == -1)
            {
                fprintf (stderr, "error %d read fail: %s\n", errno,  strerror (errno));
                break;
            }

            if (first_packet && rx_start)
            {
                gettimeofday (&ts_actual, NULL);
                if (1000000 * (ts_actual.tv_sec - ts_start.tv_sec) + (ts_actual.tv_usec - ts_start.tv_usec) > num_packets * inter_packet_interval)
                    break;
            }
        }
        printf ("total: reveive %d packets, %.2f%% loss\n", rx_counter, 100 * (float)(num_packets-rx_counter)/(float)num_packets);
    }
    return 0;
}
