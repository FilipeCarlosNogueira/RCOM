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

  unsigned char *file_name = NULL;
  int size_file_name = 0;

  off_t file_size; //em bytes
  unsigned char *msg;

  int size_control_package = 0;

  unsigned char *start_package, *end_package;

  int size_packet = defined_size_packet;

  off_t index = 0;
  unsigned char *packet;
  int header_size;
  unsigned char *header_msg;

  struct timespec clock_start, clock_end;

  double total_time;

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

  setTermios(fd);

  // Alarm Handler funtion
  (void) signal(SIGALRM, alarm_handler);

  // Save file name and the file name length
  printf("\n*File info*\n");
  size_file_name = strlen(argv[2]);
  if((file_name = (unsigned char *)malloc(size_file_name)) == NULL){
    perror("file_name malloc failed!");
    exit(-1);
  }
  file_name = (unsigned char *)argv[2];
  printf("File name: %s\n", file_name);

  // Open and read file
  msg = open_file((unsigned char *)argv[2], &file_size);
  printf("File size: %lld bytes\n\n", file_size);

  // Start clock
  clock_gettime(CLOCK_REALTIME, &clock_start);

  /* --- Data connection establishment --- */
  printf("*Data connection establishment*\n");
  if (!llopen(fd, TRANSMITTER)){
    perror("Connection failed!");
    return -1;
  }

  /* --- Data transference --- */
  printf("*Data transference*\n");

  printf("\n--Sending trama START...\n");
  start_package = control_package(C2_start, file_size, file_name, size_file_name, &size_control_package);
  if(!llwrite(fd, start_package, size_control_package)){
    perror("Sending Control Package START failed!");
    exit(-1);
  } printf("--Sent trama START.\n");

  srand(time(NULL));

  printf("\n--Splitting file and sending packages..\n");
  while (size_packet == defined_size_packet && index < file_size){
    
    packet = split_msg(msg, &index, &size_packet, file_size);
    printf("Sent packet number %d\n", total_tramas);

    header_size = size_packet;
    header_msg = header(packet, file_size, &header_size);
    
    if (!llwrite(fd, header_msg, header_size)){
      perror("Reached alarms limit!\n");
      exit(-1);
    }
  }
  printf("--File sent.\n");

  printf("\n--Sending Trama END..\n");
  end_package = control_package(C2_end, file_size, file_name, size_file_name, &size_control_package);
  if(!llwrite(fd, end_package, size_control_package)){
    perror("Sending Control Package END failed!");
    exit(-1);
  } printf("--Sent trama END.\n");

  /* --- Termination --- */
  printf("\n*Termination*\n");
  llclose(fd, TRANSMITTER);

  printf("\nTransmitter terminated!\n");

  // Stop clock
  clock_gettime(CLOCK_REALTIME, &clock_end);

  total_time = (clock_end.tv_sec - clock_start.tv_sec) + (clock_end.tv_nsec - clock_start.tv_nsec) / 1E9;
  printf("\nTime passed (in seconds): %f\n", total_time);

  sleep(2);

  close(fd);

  return 0;
}
