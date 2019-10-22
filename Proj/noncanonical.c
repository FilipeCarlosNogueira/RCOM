/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define C 0x03
#define C_UA 0x07
#define C10 0x00
#define C11 0x40
#define C_RR0 0x05
#define C_RR1 0x85
#define C_REJ0 0x01
#define C_REJ1 0x81
#define C2_end 0x03
#define DISC 0x0B

#define esc 0x7D
#define esc_flag 0x5E
#define esc_escape 0x5D

volatile int STOP = FALSE;

struct termios oldtio, newtio;

int test = 0;

/*int fd;
char res;
char message[5];
unsigned char UA[5];*/

unsigned char read_SET(int fd, unsigned char c_set)
{
	int state = 0;
	unsigned char c;

	while (state != 5)
	{
		read(fd, &c, 1);
		switch (state)
		{
		case 0: //analises flag
			if (c == FLAG)
				state = 1;
			break;
		case 1: //analises A
			if (c == A)
				state = 2;
			else
			{
				if (c == FLAG)
					state = 1;
				else
					state = 0;
			}
			break;
		case 2: //analises C
			if (c == c_set)
				state = 3;
			else
			{
				if (c == FLAG)
					state = 1;
				else
					state = 0;
			}
			break;
		case 3: //analises BCC1
			if (c == (A ^ c_set))
				state = 4;
			else
				state = 0;
			break;
		case 4: //analises final flag
			if (c == FLAG)
				state = 5;
			else
				state = 0;
			break;
		}
	}
	return TRUE;
}

void send_SET(int fd, unsigned char c_ua)
{
	unsigned char set[5];

	set[0] = FLAG;
	set[1] = A;
	set[2] = c_ua;
	set[3] = set[1] ^ set[2];
	set[4] = FLAG;

	write(fd, set, 5);
}

int BCC2_test(unsigned char* msg, int msg_size)
{
	int i = 1;
	unsigned char BCC2 = msg[0];

	while(i < msg_size - 1)
	{
		BCC2 ^= msg[i];
		i++;
	}

	if (BCC2 == msg[msg_size - 1])
		return TRUE;
	else
		return FALSE;
}

unsigned char* start_filename(unsigned char* start)
{
	int L2 = (int)start[8];
	unsigned char* name = (unsigned char*)malloc(L2 + 1);

	int i;

	for (i = 0; i < L2; i++)
	{
		name[i] = start[9 + i];
	}

	name[L2] = '\0';

	return name;
}

off_t start_file_size(unsigned char* start)
{
	return (start[3] << 24) | (start[4] << 16) | (start[5] << 8) | (start[6]);
}

int final_msg_check(unsigned char* start, int start_size, unsigned char* final, int final_size)
{
	int s = 1;
	int f = 1;

	if (start_size != final_size)
		return FALSE;
	else
	{
		if (final[0] == C2_end)
		{
			while(s < start_size)
			{
				if (start[s] != final[f])
					return FALSE;

				s++;
				f++;
			}
			return TRUE;
		}
		else
			return FALSE;
	}
}

unsigned char* remove_header(unsigned char* remove, int remove_size, int* removed_size)
{
	int i = 0;
	int j = 4;

	unsigned char* msg_removed_header = (unsigned char*)malloc(remove_size - 4);

	while(i < remove_size)
	{
		msg_removed_header[i] = remove[j];

		i++;
		j++;
	}

	*removed_size = remove_size - 4;

	return msg_removed_header;
}

void new_file(unsigned char* msg, off_t* file_size, unsigned char filename[])
{
	FILE* file = fopen((char*)filename, "wb+");

	fwrite((void*)msg, 1, *file_size, file);

	printf("%zd\n", *file_size);
	printf("New file generated!\n");

	fclose(file);
}

int llopen(int fd)
{
	if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 1;   /* inter-character timer used */
	newtio.c_cc[VMIN] = 0;   /* doesn't block read */

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	printf("New termios structure set\n");

	if (read_SET(fd, C))
	{
		printf("SET received!\n");
		send_SET(fd, C_UA);
		printf("UA sent!\n");
	}
}

unsigned char* llread(int fd, int* msg_size)
{
	unsigned char* msg = (unsigned char*)malloc(0);
	*msg_size = 0;
	unsigned char read_c;
	unsigned char c;
	int trama = 0;
	int send_data = FALSE;
	int state = 0;

	while (state != 6)
	{
		read(fd, &c, 1);
		switch (state)
		{
		case 0:
			if (c == FLAG)
				state = 1;
			break;
		case 1:
			if (c == A)
				state = 2;
			else
			{
				if (c == FLAG)
					state = 1;
				else
					state = 0;
			}
			break;
		case 2:
			if (c == C10)
			{
				trama = 0;
				read_c = c;
				state = 3;
			}
			else if (c == C11)
			{
				trama = 1;
				read_c = c;
				state = 3;
			}
			else
			{
				if (c == FLAG)
					state = 1;
				else
					state = 0;
			}
			break;
		case 3:
			if (c == (A ^ read_c))
				state = 4;
			else
				state = 0;
			break;
		case 4:
			if (c == FLAG)
			{
				if (BCC2_test(msg, *msg_size))
				{
					if (trama == 0)
						send_SET(fd, C_RR1);
					else
						send_SET(fd, C_RR0);

					state = 6;
					send_data = TRUE;
					printf("RR sent, trama -> %d\n", trama);
				}
				else
				{
					if (trama == 0)
						send_SET(fd, C_REJ1);
					else
						send_SET(fd, C_REJ0);
					state = 6;
					send_data = FALSE;
					printf("REJ sent, trama -> %d\n", trama);
				}
			}
			else if (c == esc)
			{
				state = 5;
			}
			else
			{
				msg = (unsigned char*)realloc(msg, ++(*msg_size));
				msg[*msg_size - 1] = c;
			}
			break;
		case 5:
			if (c == esc_flag)
			{
				msg = (unsigned char*)realloc(msg, ++(*msg_size));
				msg[*msg_size - 1] = FLAG;
			}
			else
			{
				if (c == esc_escape)
				{
					msg = (unsigned char*)realloc(msg, ++(*msg_size));
					msg[*msg_size - 1] = esc;
				}
				else
				{
					perror("Invalid character after esc character");
					exit(-1);
				}
			}
			state = 4;
			break;
		}
	}

	printf("Message size = %d\n", *msg_size);

	msg = (unsigned char*)realloc(msg, *msg_size - 1);
	*msg_size = *msg_size - 1;

	if (send_data)
	{
		if (trama == test)
			test ^= 1;
		else
			*msg_size = 0;
	}
	else
		*msg_size = 0;
	return msg;
}

void llclose(int fd)
{
	read_SET(fd, DISC);
	printf("DISC received...\n");

	send_SET(fd, DISC);
	printf("Sent DISC...\n");

	read_SET(fd, C_UA);
	printf("UA received...\n");

	printf("Receiver terminated!\n");

	tcsetattr(fd, TCSANOW, &oldtio);
}

int main(int argc, char** argv)
{
	int fd;
	int msg_size = 0;
	unsigned char* msg_ready;
	int start_size = 0;
	unsigned char* start;
	off_t file_size = 0;
	unsigned char* file;
	off_t index = 0;

	if ((argc < 2) ||
		((strcmp("/dev/ttyS0", argv[1])!=0) &&
		 (strcmp("/dev/ttyS1", argv[1])!=0) )) {
		//((strcmp("/tmp/rcom0", argv[1]) != 0) &&
		//(strcmp("/tmp/rcom1", argv[1]) != 0))) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}

	/*
	  Open serial port device for reading and writing and not as controlling tty
	  because we don't want to get killed if linenoise sends CTRL-C.
	*/

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0) { perror(argv[1]); exit(-1); }

	if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	llopen(fd);

	start = llread(fd, &start_size);

	unsigned char* filename = start_filename(start);
	file_size = start_file_size(start);

	file = (unsigned char*)malloc(file_size);

	while (TRUE)
	{
		msg_ready = llread(fd, &msg_size);
		if (msg_size == 0)
			continue;
		if (final_msg_check(start, start_size, msg_ready, msg_size))
		{
			printf("Final message received!\n");
			break;
		}

		int noHeader_size = 0;

		msg_ready = remove_header(msg_ready, msg_size, &noHeader_size);

		memcpy(file + index, msg_ready, noHeader_size);
		index += noHeader_size;
	}

	printf("Message: \n");

	int i = 0;
	while(i < file_size)
	{
		printf("%x", file[i]);
		i++;
	}

	new_file(file, &file_size, filename);

	llclose(fd);
	sleep(1);

	close(fd);
	
	return 0;
}