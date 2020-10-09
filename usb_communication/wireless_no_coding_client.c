#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

#include "serial.h"
#include "utils.h"
#include "lowpan.h"
#include "reassemble.h"
#include "config.h"

#define USB_DEVICE  "/dev/ttyACM0"
#define LOG_FILE    "log.dump"
#define MAX_SIZE    128

static struct option long_options[] =
{
    {"port",        required_argument, 0, 'p'},
    {"symbolSize",  required_argument, 0, 's'},
    {"genSize",     required_argument, 0, 'g'},
    {"logFile",     required_argument, 0, 'l'},
    {"help",        no_argument,       0, 'h'},
    {0, 0, 0, 0}
};

void usage(void)
{
    printf ("Usage: [-p --port <serial port number>] [-s --symbolSize <symbol size>] [-g --genSize <generation size>] [-l --logFile <log file name>] [-h --help]\n");
    printf ("Options:\n");
    printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
    printf ("\t-s --symbolSize\tpayload size\t\t\tDefault: 4\n");
    printf ("\t-g --genSize\tnumber of packets to be sent\t\t\tDefault: 10\n");
    printf ("\t-l --logFile\tlog file name\t\t\tDefault: log.dump\n");
    printf ("\t-h --help\tthis help documetation\n");
}

int write_measurement_log (char* log_file_name,
                           uint16_t tx_frame_count,
                           uint32_t symbol_size,
                           uint32_t generation_size,
                           bool tx_success)
{
    FILE* fp;
    float loss_rate = 0;
    fp = fopen (log_file_name, "a+");
    if (fp == NULL)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, log_file_name, strerror (errno));
        return -1;
    }
    if (tx_success == false)
        loss_rate = 1;

    fprintf(fp, "{\"type\": \"no_coding\", \"data_size\": %u, \"loss_rate\": %.2f, \"tx_num\": %u },\n",
            symbol_size * generation_size,
            loss_rate,
            tx_frame_count);
    fclose(fp);
    return 0;
}


int main(int argc, char *argv[])
{
    char* serial_port = (char*)USB_DEVICE;
    char* log_file_name = (char*)LOG_FILE;
    uint32_t symbol_size = 4;
    uint32_t generation_size = 10;

    // cmd arguments parsing
    int opt;
    int option_index = 0;

    while ((opt = getopt_long (argc, argv, "p:s:g:l:h", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
            case 'p':
                serial_port = optarg;
                break;
            case 's':
                symbol_size = atoi (optarg);
                break;
            case 'g':
                generation_size = atoi (optarg);
                break;
            case 'l':
                log_file_name = optarg;
                break;
            case 'h':
                usage ();
                return 0;
            default:
                usage ();
                return 0;
        }
    }

    // Seed random number generator to produce different results every time
    srand (static_cast<uint32_t> (time (0)));

    // USB communication
    int fd = open_serial_port (serial_port, B115200, 0);
    if (fd < 0)
    {
        fprintf (stderr, "error %d opening %s: %s\n", errno, serial_port, strerror (errno));
        return -1;
    }

    int ret;
    uint8_t rx_buf[MAX_SIZE];
    memset (rx_buf, 0, sizeof rx_buf);
    uint16_t tx_packet_count = 0;
    uint16_t tx_frame_count = 0;
    uint8_t tx_frame_tries = 0;
    uint16_t ack_rx_num = 0;
    uint16_t data_offset = 0;
    uint16_t ack_timeout = 50; // ack timeout in ms
    //uint32_t inter_frame_interval = 50000; // inter frame interval in us
    bool tx_frame_success = false;
    bool tx_data_success = false;

    // set buffers
    uint8_t data_in[symbol_size * generation_size];
    // fill source data buffer with specific values
    memset (data_in, 'T', sizeof (data_in));

    // set ack packet buffer
    uint8_t ack_packet[MAX_SIZE];
    memset (ack_packet, 0, sizeof ack_packet);

    // set data packet buffer
    uint32_t packet_length = symbol_size +
                             IPHC_TOTAL_SIZE +
                             UDPHC_TOTAL_SIZE;
    uint8_t packet[packet_length];
    memset (packet, 0, sizeof packet);
    virtual_packet_t tx_packet[MAX_FRAG_NUM];
    memset (tx_packet, 0, sizeof (virtual_packet_t) * MAX_FRAG_NUM);

    // client operations
    while (tx_packet_count < generation_size)
    {
        // construct packet
        set_ip_header (packet);
        set_udp_header (packet + IPHC_TOTAL_SIZE);
        memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                data_in + data_offset,
                symbol_size);
        // fragmentation
        if (need_fragmentation (packet_length) == true)
            do_fragmentation (tx_packet, packet, packet_length);
        else
            generate_normal_packet (&tx_packet[0], packet, packet_length);

        // send a packet
        if (need_fragmentation (packet_length) == true)
        {
            for (uint8_t j = 0; j < get_fragment_num(); j++)
            {
                // reset MAC tries counter
                tx_frame_tries = 0;
                // reset flag
                tx_frame_success = false;
                while (tx_frame_tries < (MAC_MAX_RETRIES + 1))
                {
                    printf ("send a frame\n");
                    ret = write_serial_port (fd, tx_packet[j].packet, tx_packet[j].length);
                    if (ret < 0)
                        return -1;
                    tx_frame_count++;
                    tx_frame_tries++;
                    // wait for ack
                    tx_frame_success = wait_ack (fd, ack_timeout, (char*)SERVER_ACK);
                    if (tx_frame_success == true)
                    {
                        printf ("receive ACK from server\n");
                        ack_rx_num++;
                        break;
                    }
                }
                // if previous frame fails, subsequent frames won't be sent
                if (tx_frame_success == false)
                {
                    data_offset += symbol_size;
                    tx_packet_count++;
                    break;
                }
            }
            if (ack_rx_num == get_fragment_num())
                tx_packet_count++;
        }
        else
        {
            // reset MAC tries counter
            tx_frame_tries = 0;
            // reset flag
            tx_frame_success = false;
            while (tx_frame_tries < (MAC_MAX_RETRIES + 1))
            {
                printf ("send a packet\n");
                ret = write_serial_port (fd, tx_packet[0].packet, tx_packet[0].length);
                if (ret < 0)
                    return -1;
                tx_frame_count++;
                tx_frame_tries++;
                // wait for ack
                tx_frame_success = wait_ack (fd, ack_timeout, (char*)SERVER_ACK);
                if (tx_frame_success == true)
                {
                    printf ("receive ACK from server\n");
                    ack_rx_num++;
                    break;
                }
            }
            data_offset += symbol_size;
            tx_packet_count++;
        }
        // reset packet buffer
        memset (packet, 0, sizeof packet);
    } // end of while
    printf ("packet total send: %u\n", tx_packet_count);
    printf ("frame total send: %u\n", tx_frame_count);
    printf ("ack total receive: %u\n", ack_rx_num);

    // check if transmission is successful
    if (need_fragmentation (packet_length) == true &&
        ack_rx_num / generation_size == get_fragment_num())
        tx_data_success = true;
    else if (need_fragmentation (packet_length) == false &&
             ack_rx_num == generation_size)
        tx_data_success = true;

    // write log to json file
    write_measurement_log (log_file_name,
                           tx_frame_count,
                           symbol_size,
                           generation_size,
                           tx_data_success);
    return 0;
}
