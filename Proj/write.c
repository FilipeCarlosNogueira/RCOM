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

int main(int argc, char** argv){
  
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
  unsigned char *file_data;

  int size_control_package = 0;

  unsigned char *start_package, *end_package;

  unsigned char *split_packet;
  int size_split_packet = split_packet_size;

  off_t index = 0;
  unsigned char *application_packet;
  int application_packet_size;

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
  file_data = open_file((unsigned char *)argv[2], &file_size);
  printf("File size: %lld bytes\n\n", file_size);

  // Start clock
  clock_gettime(CLOCK_REALTIME, &clock_start);

  /* --- Data connection establishment --- */
  printf("*Data connection establishment*\n");
  if (llopen(fd, TRANSMITTER) == -1){
    perror("Data Connection failed!");
    return -1;
  }

  /* --- Data transference --- */
  printf("*Data transference*\n");

  // Start Trama
  printf("\n--> Sending trama START...\n");
  start_package = control_package(C2_start, file_size, file_name, size_file_name, &size_control_package);
  if(!llwrite(fd, start_package, size_control_package)){
    perror("Sending Control Package START failed!");
    exit(-1);
  } printf("--> Sent trama START.\n");

  srand(time(NULL));

  // Information Tramas
  printf("\n--> Splitting file and sending packages..\n");
  while (size_split_packet == split_packet_size && index < file_size){
    
    split_packet = split_msg(file_data, &index, &size_split_packet, file_size);

    printf("Sending packet number %d..\n", total_tramas);

    application_packet_size = size_split_packet;
    application_packet = header(split_packet, &application_packet_size);
    
    if (!llwrite(fd, application_packet, application_packet_size)){
      perror("Reached alarms limit!\n");
      exit(-1);
    }
  }
  printf("--> File sent.\n");

  // End Trama
  printf("\n--> Sending Trama END..\n");
  end_package = control_package(C2_end, file_size, file_name, size_file_name, &size_control_package);
  if(!llwrite(fd, end_package, size_control_package)){
    perror("Sending Control Package END failed!");
    exit(-1);
  } printf("--> Sent trama END.\n");

  /* --- Termination --- */
  printf("\n*Termination*\n");
  llclose(fd, TRANSMITTER);

  // Stop clock
  clock_gettime(CLOCK_REALTIME, &clock_end);

  printf("\nTransmitter terminated!\n");

  total_time = (clock_end.tv_sec - clock_start.tv_sec) + (clock_end.tv_nsec - clock_start.tv_nsec) / 1E9;
  printf("\nTime passed (in seconds): %f\n", total_time);

  sleep(2);

  close(fd);

  return 0;
}
