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

#include "macros.h"
#include "utils.h"
#include "dataLink.h"
#include "application.h"

int main(int argc, char** argv)
{
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

	int fd;
	int msg_size = 0;
	unsigned char* msg_ready;
	int start_size = 0;
	unsigned char* start;
	off_t file_size = 0;
	unsigned char* file;
	off_t index = 0;

	/*
	  Open serial port device for reading and writing and not as controlling tty
	  because we don't want to get killed if linenoise sends CTRL-C.
	*/

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0) { perror(argv[1]); exit(-1); }

	setTermios(fd);

	/* --- Data connection establishment --- */
	printf("\n*Data connection establishment*\n");
	if (!llopen(fd, RECEIVER)){
		perror("Connection failed!");
		return -1;
	}

	/* --- Data transference --- */
	printf("*Data transference*\n");

	printf("\n--Reading trama START...\n");
	start = llread(fd, &start_size);

	unsigned char* filename = start_filename(start);
	file_size = start_file_size(start);

	if((file = (unsigned char*)malloc(file_size)) == NULL){
		perror("file malloc failed!");
		exit(-1);
	}
	printf("--Trama START processed.\n");

	printf("\n--Reading split messages..\n");
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
	printf("--Split messages processed.\n");

	printf("\nMessage: ");
	for(int i = 0; i < file_size; ++i)
		printf("%x", file[i]);
	printf("\n");

	printf("\n*New file*\n");
	new_file(file, &file_size, filename);
	printf("New file name: %s\n", filename);
	printf("New file size: %lld\n", file_size);

	/* --- Termination --- */
	printf("\n*Termination*\n");
	llclose(fd, RECEIVER);

	printf("\nReceiver terminated!\n");
	
	sleep(1);

	close(fd);
	
	return 0;
}