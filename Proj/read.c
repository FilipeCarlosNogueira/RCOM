﻿/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "utils.h"
#include "dataLink.h"

/* ----------------- Application Layer ---------------- */

unsigned char* start_filename(unsigned char* start)
{
	int auxL2 = (int)start[8];
	unsigned char* name = (unsigned char*)malloc(auxL2 + 1);

	int i;

	for (i = 0; i < auxL2; i++)
	{
		name[i] = start[9 + i];
	}

	name[auxL2] = '\0';

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

	printf("%lld\n", *file_size);
	printf("New file generated!\n");

	fclose(file);
}

/* ----------------- Main ----------------- */
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

	#ifdef UNIX
        if ( (argc < 2) ||
             ((strcmp("/dev/ttyS0", argv[1])!=0) &&
              (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
        }
    #elif __APPLE__
        if ( (argc < 2) ||
            ((strcmp("/tmp/rcom0", argv[1])!=0) &&
            (strcmp("/tmp/rcom1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/rcom0\n");
        exit(1);
        }
    #endif

	/*
	  Open serial port device for reading and writing and not as controlling tty
	  because we don't want to get killed if linenoise sends CTRL-C.
	*/

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0) { perror(argv[1]); exit(-1); }

	/*
	if (tcgetattr(fd, &oldtio) == -1) { // save current port settings
		perror("tcgetattr");
		exit(-1);
	}
	*/

	llopen(fd, RECEIVER);

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

	llclose(fd, RECEIVER);
	sleep(1);

	close(fd);
	
	return 0;
}