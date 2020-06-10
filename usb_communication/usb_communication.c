#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>

#include "serial.h"

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128

static struct option long_options[] =
{
	{"port", required_argument, 0, 'p'},
	{"mode", required_argument, 0, 'm'},
	{"help", no_argument, 		0, 'h'},
	{0, 0, 0, 0}
};

void usage(void)
{
	printf ("Usage: [-p --port <serial port number>] [-m --mode <mode>] [-h --help]\n");
	printf ("Options:\n");
	printf ("\t-p --port\tserial port number to open\tDefault: /dev/ttyACM0\n");
	printf ("\t-m --mode\trunning mode\t\t\tOptions: client/server/echo\n");
	printf ("\t-h --help\tthis help documetation\n");
}


int main(int argc, char *argv[])
{
	char str[MAX_SIZE];
	char rx_buf[MAX_SIZE];
	bool client = false;
	bool server = false;
	bool echo = false;
	char* serial_port = (char*)USB_DEVICE;

	// cmd arguments parsing
	int opt;
	int option_index = 0;

	if (argc == 1)
	{
		usage();
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
				if (!strcmp(optarg, "client"))
					client = true;
				else if (!strcmp(optarg, "server"))
					server = true;
				else if (!strcmp(optarg, "echo"))
					echo = true;
				else
				{
					fprintf (stderr, "error: unknown mode %s\n", optarg);
					return -1;
				}
				break;
			case 'h':
				usage();
				return 0;
				break;
			default:
				usage();
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

	int tx_counter = 0;
    int rx_num;
    int tx_ret;
	while (true)
	{
		if (client || echo)
		{
			printf ("enter a value :");
			scanf("%[^\n]%*c", str);
			tx_ret = write (fd, str, strlen(str));
            if (tx_ret > 0)
            {
			    printf ("%d write %d bytes\n", tx_counter, tx_ret);
                tx_counter++;
            }
            else
            {
                fprintf (stderr, "error %d write fail: %s\n", errno, strerror (errno));
                break;
            }
		}
		usleep(1000);
		if (server || echo)
		{
			rx_num = read (fd, rx_buf, MAX_SIZE);
			if (rx_num > 0)
			{
				printf ("receive %d bytes: %s\n", rx_num, rx_buf);
				// clear rx buffer after processing
				memset (rx_buf, 0, sizeof rx_buf);
			}
            else if (rx_num == -1)
            {
                fprintf (stderr, "error %d read fail: %s\n", errno, strerror (errno));
                break;
            }
		}
	}
	return 0;
}
