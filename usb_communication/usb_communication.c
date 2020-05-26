#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "serial.h"

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128


int main(int argc, char *argv[])
{
	char str[MAX_SIZE];
	char rx_buf[MAX_SIZE];
	bool client = false;
	bool server = false;
	bool echo = false;

	if (!strcmp(argv[2], "client"))
		client = true;
	else if (!strcmp(argv[2], "server"))
		server = true;
	else if (!strcmp(argv[2], "echo"))
		echo = true;
	else
		fprintf (stderr, "error: unknown mode %s\n", argv[2]);
	

	int fd = open_serial_port (argv[1], B115200, 0);
	if (fd < 0)
	{
		fprintf (stderr, "error %d opening %s: %s\n", errno, USB_DEVICE, strerror (errno));
		return -1;
	}

	while (true)
	{
		if (client || echo)
		{
			printf ("enter a value :");
			scanf("%[^\n]%*c", str);
			write (fd, str, strlen(str));
		}
		usleep(1000);
		if (server || echo)
		{
			int rx_num = read (fd, rx_buf, MAX_SIZE);
			if (rx_num > 0)
			{
				printf ("receive %d bytes: %s\n", rx_num, rx_buf);
				// clear rx buffer after processing
				memset (rx_buf, 0, sizeof rx_buf);
			}
		}
	}
	return 0;
}
