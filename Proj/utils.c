#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>

#include "macros.h"

struct termios oldtio, newtio;
int alarm_counter = 0;
bool alarm_flag = TRUE;

/*
* Opens serial port.
* Builds termios 
* @param **argv
*/
void setTermios(int fd){

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */

  /*
      VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
      leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("\n*New termios structure set*\n");
}

/*
* Alarm handler funtion
*/
void alarm_handler(){
	alarm_flag=TRUE;
	++alarm_counter;
  printf("alarme # %d\n", alarm_counter);
}

/*
* Opens file, reads and saves its content.
* Returns everything that was saved.
* @param *file_name, *file_size
* return *file_data
*/
unsigned char *open_file(unsigned char *file_name, off_t *file_size){
	FILE *fd;
	struct stat data;
	unsigned char *file_data;

	if ((fd = fopen((char *)file_name, "rb")) == NULL){
		perror("Error opening file!");
		exit(-1);
	}

	// Calculate the size of the file.
	stat((char *)file_name, &data);
	(*file_size) = data.st_size;

	file_data = (unsigned char *)malloc(*file_size);
	if(file_data == NULL){
		perror("file_data malloc failed!");
		exit(-1);
	}

	// Read file and save its content in "file_data"
	if(fread(file_data, sizeof(unsigned char), *file_size, fd) != *file_size){
		perror("fread() of file failed!");
		exit(-1);
	}

	return file_data;
}

/*
*
*/
void new_file(unsigned char* msg, off_t* file_size, unsigned char filename[]){
	FILE* new_file = fopen((char*)filename, "wb+");

	fwrite((void*)msg, 1, *file_size, new_file);

	fclose(new_file);
}
