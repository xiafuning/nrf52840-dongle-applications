#include <stdio.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define USB_DEVICE "/dev/ttyACM0"
#define MAX_SIZE 128

int set_interface_attributes (int fd, int speed, int parity)
{
	struct termios tty;
	memset(&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		fprintf (stderr, "error %d from tcgetattr\n", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	tty.c_iflag &= ~IGNBRK;         				// disable break processing
	tty.c_lflag &= ~ICANON;							// disable canonical mode
	tty.c_lflag &= ~ECHO; 							// disable echo
	tty.c_lflag &= ~ISIG; 							// disable interpretation of INTR, QUIT and SUSP
	tty.c_oflag = 0;                				// no remapping, no delays
	tty.c_cc[VMIN]  = 0;            				// read doesn't block
	tty.c_cc[VTIME] = 5;            				// 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); 		// shut off xon/xoff ctrl
	tty.c_cflag |= (CLOCAL | CREAD);				// ignore modem controls, enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      		// shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;							// clear stop field(only one stop bit)
	tty.c_cflag &= ~CRTSCTS;						// disable RTS/CTS hardware flow control

	tty.c_oflag &= ~OPOST; 
	tty.c_oflag &= ~ONLCR;

	// save tty settings and check for error
	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		fprintf (stderr, "error %d from tcsetattr\n", errno);
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		fprintf (stderr, "error %d from tggetattr\n", errno);
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		fprintf (stderr, "error %d setting term attributes\n", errno);
}

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
	

	int fd = open (argv[1], O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
		fprintf (stderr, "error %d opening %s: %s\n", errno, USB_DEVICE, strerror (errno));
		return -1;
	}

	set_interface_attributes (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (fd, 0);                		// set no blocking

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
