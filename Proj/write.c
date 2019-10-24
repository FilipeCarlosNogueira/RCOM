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
#include "application.h"

int main(int argc, char** argv){
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

  // Alarm Handler funtion
  (void) signal(SIGALRM, alarm_handler);

  msg = open_file((unsigned char *)argv[2], &file_size);

  //start clock
  clock_gettime(CLOCK_REALTIME, &clock_start);

  setTermios(fd);

  if (!llopen(fd, TRANSMITTER)){
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
