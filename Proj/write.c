/*Non-Canonical Input Processing*/

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

#include "utils.h"
#include "dataLink.h"

unsigned char msg_counter = 0;

/* ----------------- Application Layer ---------------- */

unsigned char *open_file(unsigned char *file_name, off_t *file_size)
{
  FILE *f;
  struct stat data;
  unsigned char *file_data;

  if ((f = fopen((char *)file_name, "rb")) == NULL)
  {
    perror("Error opening file!");
    exit(-1);
  }

  stat((char *)file_name, &data);
  (*file_size) = data.st_size;

  printf("This file has %lld bytes \n", *file_size);

  file_data = (unsigned char *)malloc(*file_size);

  fread(file_data, sizeof(unsigned char), *file_size, f);
  
  return file_data;
}

unsigned char *control_package(unsigned char state, off_t file_size, unsigned char *file_name, int size_file_name, int *size_control_package)
{
  *size_control_package = 9 * sizeof(unsigned char) + size_file_name;
  unsigned char *package = (unsigned char *)malloc(*size_control_package);

  if (state == C2_start)
    package[0] = C2_start;
  else
    package[0] = C2_end;
  
  package[1] = T1;
  package[2] = L1;
  package[3] = (file_size >> 24) & 0xFF;
  package[4] = (file_size >> 16) & 0xFF;
  package[5] = (file_size >> 8) & 0xFF;
  package[6] = file_size & 0xFF;
  package[7] = T2;
  package[8] = size_file_name;

  int i = 0;
  while(i < size_file_name)
  {
    package[9 + i] = file_name[i];
    i++;
  }
  return package;
}

unsigned char *split_msg(unsigned char *msg, off_t *index, int *size_packet, off_t file_size)
{
  unsigned char *packet;
  int i = 0;
  off_t j = *index;

  if (*index + *size_packet > file_size)
    *size_packet = file_size - *index;

  packet = (unsigned char *)malloc(*size_packet);

  while(i < *size_packet)
  {
    packet[i] = msg[j];
    i++;
    j++;
  }

  *index = j;
  
  return packet;
}

unsigned char* header(unsigned char* msg, off_t file_size, int* packet_size)
{
	unsigned char* final_msg = (unsigned char*)malloc(file_size + 4);

	final_msg[0] = C_header;
	final_msg[1] = msg_counter % 255;
	final_msg[2] = (int)file_size / 256;
	final_msg[3] = (int)file_size % 256;

	memcpy(final_msg + 4, msg, *packet_size);
	*packet_size += 4;

	msg_counter++;
	total_tramas++;

	return final_msg;
}

/* ----------------- Main ----------------- */
int main(int argc, char** argv)
{
  // char buf[255];
  // int i, sum = 0, speed = 0, 
  int fd;
  unsigned char *msg;
  off_t file_size; //em bytes
  off_t index = 0;
  struct timespec clock_start, clock_end;

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

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

  (void) signal(SIGALRM, alarm_handler);  // instala  rotina que atende interrupcao

  msg = open_file((unsigned char *)argv[2], &file_size);

  clock_gettime(CLOCK_REALTIME, &clock_start); //start clock

  setTermios(fd);
  
  if (!llopen(fd, TRANSMITTER))
  {
    perror("Connection failed!");
    return -1;
  }

  int size_file_name = strlen(argv[2]);
  int size_control_package = 0;
  unsigned char *file_name = (unsigned char *)malloc(size_file_name);
  file_name = (unsigned char *)argv[2];
  unsigned char *start = control_package(C2_start, file_size, file_name, size_file_name, &size_control_package);

  llwrite(fd, start, size_control_package);
  printf("Sent trama START...\n");

  int size_packet = defined_size_packet;
  srand(time(NULL));

  while (size_packet == defined_size_packet && index < file_size)
  {
    unsigned char *packet = split_msg(msg, &index, &size_packet, file_size);
    printf("Sent packet number %d\n", total_tramas);
  
    int header_size = size_packet;
    unsigned char *header_msg = header(packet, file_size, &header_size);
    
    if (!llwrite(fd, header_msg, header_size))
    {
      printf("Reached alarms limit!\n");
      return -1;
    }
  }

	unsigned char *end = control_package(C2_end, file_size, file_name, size_file_name, &size_control_package);

  llwrite(fd, end, size_control_package);
  printf("Sent trama END...\n");

  llclose(fd, TRANSMITTER);

  clock_gettime(CLOCK_REALTIME, &clock_end); //stop clock

  double total_time = (clock_end.tv_sec - clock_start.tv_sec) + (clock_end.tv_nsec - clock_start.tv_nsec) / 1E9;

  printf("Time passed (in seconds): %f\n", total_time);

  sleep(2);

  close(fd);
  
  return 0;
}
