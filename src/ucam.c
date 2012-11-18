#include <stdio.h>
#include <string.h>
#include <unistd.h> // TODO : remove
#include "ucam.h"
#include "serial.h"

// Following commands are only structure basis, fields need to be filled up
const unsigned char CMD_SYNC[]		= {0xAA, 0x0D, 0x00, 0x00, 0x00, 0x00};
const unsigned char ACK[]		= {0xAA, 0x0E, 0x00, 0x00, 0x00, 0x00};
const unsigned char INITIAL[]		= {0xAA, 0x01, 0x00, 0x07, 0x07, 0x07};
const unsigned char SET_PACKAGE_SIZE[]	= {0xAA, 0x06, 0x08, 0x00, 0x00, 0x00};
const unsigned char SNAPSHOT[]		= {0xAA, 0x05, 0x00, 0x00, 0x00, 0x00};
const unsigned char GET_PICTURE[]	= {0xAA, 0x04, 0x01, 0x00, 0x00, 0x00};
const unsigned char DATA[]		= {0xAA, 0x0A, 0x01, 0x00, 0x00, 0x00};
const unsigned char RESET[]		= {0xAA, 0x08, 0x00, 0x00, 0x00, 0x00};

int fd;

int wait_for_ack(int fd, unsigned char cmd_id, unsigned char *counter)
{
	unsigned char c;
	unsigned char buffer[6];
	unsigned char ack[CMD_SIZE];
	int i;
	
	memcpy(ack, ACK, CMD_SIZE);
	ack[2] = cmd_id;

	for (i=0;i<500000;i++) {
		read(fd, &c, 1);
		read_buffer(buffer, c);
		// print_buffer(buffer);
		if (buffer[0] == ack[0] && buffer[1] == ack[1] && buffer[2] == ack[2] &&
		    ack[4] == 0x00 && ack[5] == 0x00) {
			*counter = buffer[3];
			return 0;
		}
	}

	return -1;
}

int wait_for_data(int fd, unsigned char *data, unsigned int *size)
{
	unsigned char c;
	unsigned char buffer[6];
	int i;

	for (i=0;i<5000;i++) {
		read(fd, &c, 1);
		read_buffer(buffer, c);
		// print_buffer(buffer);
		if (buffer[0] == data[0] && buffer[1] == data[1] && buffer[2] == data[2]) {
			// *counter = buffer[3];
			printf("size : %.02X %.02X %.02X\n", buffer[3], buffer[4], buffer[5]);
			*size = 42;
			return 0;
		}
	}

	return -1;
}


int ucam_reset(unsigned char type, unsigned int special)
{
	unsigned char cmd[CMD_SIZE];
	unsigned char ack_counter;
	int ret;

	printf("Sending RESET...\n");

	memcpy(cmd, RESET, CMD_SIZE);
	cmd[3] = type;
	if (special)
		cmd[5] = 0xFF;
	write(fd, cmd, CMD_SIZE);

	if ((ret = wait_for_ack(fd, RESET[1], &ack_counter)) < 0)
		goto err;

	printf("received %.02X\n", ack_counter);
	return 0;
err:
	printf("error\n");
	return ret;
}

int ucam_initial(unsigned char color, unsigned char raw_res,
		 unsigned char jpeg_res)
{
	unsigned char cmd[CMD_SIZE];
	unsigned char ack_counter;
	int ret;

	printf("Sending INITIAL...\n");

	memcpy(cmd, INITIAL, CMD_SIZE);
	cmd[3] = color;
	cmd[4] = raw_res;
	cmd[5] = jpeg_res;

	if ((ret = send_cmd(cmd)) < 0)
		goto err;

	if ((ret = wait_for_ack(fd, INITIAL[1], &ack_counter) < 0))
		goto err;

	printf("received %.02X\n", ack_counter);
	return 0;
err:
	printf("error\n");
	return ret;
}

int ucam_set_package_size(unsigned short size)
{
	unsigned char cmd[CMD_SIZE];
	unsigned char ack_counter;
	int ret;

	printf("Sending SET_PACKAGE_SIZE...\n");

	memcpy(cmd, SET_PACKAGE_SIZE, CMD_SIZE);
	cmd[3] = 0x00; // 512 bytes
	cmd[4] = 0x02;

	if ((ret = send_cmd(cmd)) < 0)
		goto err;

	if ((ret = wait_for_ack(fd, SET_PACKAGE_SIZE[1], &ack_counter)) < 0)
		goto err;

	printf("received %.02X\n", ack_counter);
	return 0;
err:
	printf("error\n");
	return ret;
}

int ucam_snapshot(unsigned char type, unsigned short skip_frame)
{
	unsigned char cmd[CMD_SIZE];
	unsigned char ack_counter;
	int ret;

	printf("Sending SNAPSHOT...\n");

	memcpy(cmd, SNAPSHOT, CMD_SIZE);
	cmd[2] = type;
	cmd[3] = 0x00; // TODO
	cmd[4] = 0x00;

	if ((ret = send_cmd(cmd)) < 0)
		goto err;

	if ((ret = wait_for_ack(fd, SNAPSHOT[1], &ack_counter)) < 0)
		goto err;

	printf("received %.02X\n", ack_counter);
	return 0;
err:
	printf("error\n");
	return ret;
}

int ucam_get_picture(unsigned char type)
{
	unsigned char cmd[CMD_SIZE];
	unsigned char ack_counter;
	unsigned int size;
	int ret;

	printf("Sending GET_PICTURE...\n");

	memcpy(cmd, GET_PICTURE, CMD_SIZE);
	if ((ret = send_cmd(cmd)) < 0)
		goto err;

	if ((ret = wait_for_ack(fd, GET_PICTURE[1], &ack_counter)) < 0)
		goto err;

	printf("received %.02X\n", ack_counter);

	memcpy(cmd, DATA, CMD_SIZE);
	cmd[2] = 0x01;

	if ((ret = wait_for_data(fd, cmd, &size)) < 0)
		goto err;

err:
	printf("error\n");
	return ret;
}

int cmd_sync(int fd)
{
	unsigned char c;
	unsigned char buffer[6];
	int attempt = 0;
	int i;

	int ack_received = 0;
	unsigned char ack_counter;

	while (attempt < 60) {
		printf("attempt %d\n", attempt);
		write(fd, CMD_SYNC, ARRAY_SIZE(CMD_SYNC));
		// write(fd, usync, 6);
		// TODO : Use select instead
		for (i=0;i<6000;i++) {
			read(fd, &c, 1);
			// printf("%.02X ", c);
			read_buffer(buffer, c);	
			// print_buffer(buffer);
			if (buffer[0] == 0xAA && buffer[1] == 0x0E &&
			    buffer[2] == 0x0D && buffer[4] == 0x00 &&
			    buffer[5] == 0x00) {
				ack_counter = buffer[3];
				read(fd, buffer, 6);
				if (buffer[0] == 0xAA && buffer[1] == 0x0D &&
				    buffer[2] == 0x00 && buffer[3] == 0x00 &&
				    buffer[4] == 0x00 && buffer[5] == 0x00) {
					ack_received = 1;
					break;
				}
			}
			// printf("\n");
		}
		
		if (ack_received)
			break;

		attempt++;
		usleep(1000);
	}

	if (!ack_received)
		return -1;

	printf("ACK received\n");

	unsigned char cmd[6];
	memcpy(cmd, ACK, 6);
	cmd[2] = 0x0D;
	cmd[3] = ack_counter;

	write(fd, cmd, 6);

	return 0;
}

int cmd_take_picture()
{
	int ret;

	if ((ret = ucam_initial(COLOR_TYPE_JPEG, RAW_RES_640x480, JPEG_RES_640x480)) < 0)
		goto err;
	if ((ret = ucam_set_package_size(512)) < 0)
		goto err;
	if ((ret = ucam_snapshot(SNAPSHOT_TYPE_COMPRESSED, 0)) < 0)
		goto err;
	if ((ret = ucam_get_picture(PIC_TYPE_SNAPSHOT)) < 0)
		goto err;

	return 0;
err:
	printf("ERROR !!!!!!!!!!!!!!!!!!!!!!\n");
	return ret;
}

