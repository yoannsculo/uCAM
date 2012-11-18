#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h> // TODO : remove

#include "serial.h"
#include "ucam.h"

#define UCAM_TTY	"/dev/ttyUSB0"

int fd;

int main()
{
	struct termios tty;
	int ret;

	if ((fd = open (UCAM_TTY, O_RDWR | O_NONBLOCK)) < 0)
		goto err_tty;

	cfsetospeed(&tty, B115200);
	cfsetispeed(&tty, B115200);

	tty.c_cflag = CS8|CREAD|CLOCAL;

	tcsetattr(fd, TCSANOW, &tty);

	// ucam_reset(RESET_TYPE_ALL, 0);

	if ((ret = cmd_sync(fd)) < 0) {
		printf("Couldn't sync device\n");
		goto end;
	}

	// The uCAM needs this time to allow its AGC and AEC circuits to stabilise
	// otherwise the received image luminance maybe too high or too low.
	sleep(2); 

	if ((ret = cmd_take_picture()) < 0)
		goto end;
end:
	close (fd);
	return 0;
err_tty:
	printf("Couldn't open %s", UCAM_TTY);
	return -1;
}
