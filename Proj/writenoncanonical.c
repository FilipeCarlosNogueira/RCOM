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

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A 0x03
#define C 0x03
#define C_UA 0x07
#define BCC1_UA (A ^ C_UA)
#define DISC 0x0B
#define C_RR0 0x05
#define C_RR1 0x85
#define C_REJ0 0x01
#define C_REJ1 0x81
#define C2_start 0x02
#define C2_end 0x03
#define L1 0x04
#define L2 0x0B
#define T1 0x00
#define T2 0x01
#define C10 0x00
#define C11 0x40

#define MAX 3
#define TIMEOUT 3

#define esc 0x7D
#define esc_flag 0x5E
#define esc_escape 0x5D

#define defined_size_packet 100

struct termios oldtio, newtio;

int alarm_counter = 0;
int alarm_flag = FALSE;
int STOP = FALSE;
int trama = 0;
int total_tramas = 0;
unsigned char set[5];
unsigned char response[5];

void UA_statemachine(int *state, unsigned char *c)
{
  switch (*state)
  {
  case 0:  //analises flag
    if (*c == FLAG)
      *state = 1;
    break;
  case 1: //analises A
    if (*c == A)
      *state = 2;
    else
    {
      if (*c == FLAG)
        *state = 1;
      else
        *state = 0;
    }
    break;
  case 2:  //analises C
    if (*c == C_UA)
      *state = 3;
    else
    {
      if (*c == FLAG)
        *state = 1;
      else
        *state = 0;
    }
    break;
  case 3: //analises BCC1
    if (*c == BCC1_UA)
      *state = 4;
    else
      *state = 0;
    break;
  case 4: //analises final flag
    if (*c == FLAG)
    {
      STOP = TRUE;
      alarm(0);
      printf("UA recieved!\n");
    }
    else
      *state = 0;
    break;
  }
}

void send_SET(int fd, unsigned char c)
{
  unsigned char set[5];

  set[0] = FLAG;
  set[1] = A;
  set[2] = c;
  set[3] = set[1] ^ set[2];
  set[4] = FLAG;

  write(fd, set, 5);
}

void alarm_handler()                   // atende alarme
{
	printf("alarme # %d\n", alarm_counter + 1);
	alarm_flag=TRUE;
	alarm_counter++;
}

unsigned char read_SET(int fd)
{
  int state = 0;
  unsigned char c;
  unsigned char c1;

  while (!alarm_flag && state != 5)
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
      if (c == C_RR0 || c == C_RR1 || c == C_REJ0 || c == C_REJ1 || c == DISC)
      {
        c1 = c;
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
    case 3: //analises BCC1
      if (c == (A ^ c1))
        state = 4;
      else
        state = 0;
      break;
    case 4: //analises final flag
      if (c == FLAG)
      {
        alarm(0);
        state = 5;
        return c1;
      }
      else
        state = 0;
      break;
    }
  }
  return 0xFF;
}

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

  printf("This file has %ld bytes \n", *file_size);

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

unsigned char *change_BCC1(unsigned char *packet, int size_packet)
{
  unsigned char *copy = (unsigned char *)malloc(size_packet);
  memcpy(copy, packet, size_packet);
  int n = (rand() % 100) + 1;
  if (n <= 0)
  {
    int i = (rand() % 3) + 1;
    unsigned char letter = (unsigned char)('A' + (rand() % 26));
    copy[i] = letter;
    printf("BCC1 changed...\n");
  }
  return copy;
}

unsigned char *change_BCC2(unsigned char *packet, int size_packet)
{
  unsigned char *copy = (unsigned char *)malloc(size_packet);
  memcpy(copy, packet, size_packet);
  int n = (rand() % 100) + 1;
  if (n <= 0)
  {
    int i = (rand() % (size_packet - 5)) + 4;
    unsigned char letter = (unsigned char)('A' + (rand() % 26));
    copy[i] = letter;
    printf("BCC2 changed...\n");
  }
  return copy;
}

unsigned char calc_BCC2(unsigned char *msg, int size)
{
  unsigned char BCC2 = msg[0];
  int i;

  for (i = 1; i < size; i++)
  {
    BCC2 ^= msg[i];
  }

  return BCC2;
}

unsigned char *stuff_BCC2(unsigned char BCC2, int *size_BCC2)
{
  unsigned char *stuffed_BCC2;
  if (BCC2 == FLAG)
  {
    stuffed_BCC2 = (unsigned char *)malloc(2 * sizeof(unsigned char *));
    stuffed_BCC2[0] = esc;
    stuffed_BCC2[1] = esc_flag;
    (*size_BCC2)++;
  }
  else
  {
    if (BCC2 == esc)
    {
      stuffed_BCC2 = (unsigned char *)malloc(2 * sizeof(unsigned char *));
      stuffed_BCC2[0] = esc;
      stuffed_BCC2[1] = esc_escape;
      (*size_BCC2)++;
    }
  }

  return stuffed_BCC2;
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

int llopen(int fd, int x)
{
  unsigned char c;

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

  newtio.c_cc[VTIME]    = 1;   /* inter-character timer used */
  newtio.c_cc[VMIN]     = 0;   /* doesn't block read */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  while (alarm_flag && alarm_counter < MAX)
  {
    send_SET(fd, C);
    alarm(TIMEOUT);
    alarm_flag = FALSE;

    int state = 0;

    while (!STOP && !alarm_flag)
    {
      read(fd, &c, 1);
      UA_statemachine(&state, &c);
    }
  }

  printf("Alarm flag -> %d\n", alarm_flag);
  printf("Alarm counter = %d\n", alarm_counter);

  if (alarm_flag && alarm_counter == 3)
  {
    return FALSE;
  }
  else
  {
    alarm_flag = FALSE;
    alarm_counter = 0;
    return TRUE;
  }
}

int llwrite(int fd, unsigned char *msg, int size)
{
  unsigned char BCC2;
  unsigned char *BCC2_stuffed = (unsigned char *)malloc(sizeof(unsigned char));
  unsigned char *final_msg = (unsigned char *)malloc((size + 6) * sizeof(unsigned char));
  int size_final_msg = size + 6;
  int size_BCC2 = 1;
  BCC2 = calc_BCC2(msg, size);
  BCC2_stuffed = stuff_BCC2(BCC2, &size_BCC2);
  int flag = FALSE;
  int i = 0;
  int j = 4;

  final_msg[0] = FLAG;
  final_msg[1] = A;

  if (trama == 0)
  {
    final_msg[2] = C10;
  }
  else
  {
    final_msg[2] = C11;
  }
  final_msg[3] = (final_msg[1] ^ final_msg[2]);
  
  while(i < size)
  {
    if (msg[i] == FLAG)
    {
      final_msg = (unsigned char *)realloc(final_msg, ++size_final_msg);
      final_msg[j] = esc;
      final_msg[j + 1] = esc_flag;
      j = j + 2;
    }
    else
    {
      if (msg[i] == esc)
      {
        final_msg = (unsigned char *)realloc(final_msg, ++size_final_msg);
        final_msg[j] = esc;
        final_msg[j + 1] = esc_escape;
        j = j + 2;
      }
      else
      {
        final_msg[j] = msg[i];
        j++;
      }
    }

    i++;
  }

  if (size_BCC2 == 1)
    final_msg[j] = BCC2;
  else
  {
    final_msg = (unsigned char *)realloc(final_msg, ++size_final_msg);
    final_msg[j] = BCC2_stuffed[0];
    final_msg[j + 1] = BCC2_stuffed[1];
    j++;
  }
  final_msg[j + 1] = FLAG;

  do
  {
    unsigned char *aux;
    aux = change_BCC1(final_msg, size_final_msg);
    aux = change_BCC2(aux, size_final_msg);         
    write(fd, aux, size_final_msg);

    alarm_flag = FALSE;
    alarm(TIMEOUT);
    unsigned char c = read_SET(fd);
    if ((c == C_RR1 && trama == 0) || (c == C_RR0 && trama == 1))
    {
      printf("Recebeu rr %x -> trama = %d\n", c, trama);
      flag = FALSE;
      alarm_counter = 0;
      trama ^= 1;
      alarm(0);
    }
    else
    {
      if (c == C_REJ1 || c == C_REJ0)
      {
        flag = TRUE;
        printf("Recebeu rej %x -> trama=%d\n", c, trama);
        alarm(0);
      }
    }
  } while ((alarm_flag && alarm_counter < MAX) || flag);

  if (alarm_counter >= MAX)
    return FALSE;
  else
    return TRUE;
}

void llclose(int fd)
{
  unsigned char c;

  send_SET(fd, DISC);
  printf("DISC sent...\n");

  c = read_SET(fd);

  while (c != DISC)
  {
    c = read_SET(fd);
  }

  printf("DISC read...\n");

  send_SET(fd, C_UA);
  printf("Final UA sent!\n");
  printf("Emitter terminated!\n");

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }
}

int main(int argc, char** argv)
{
  char buf[255];
  int i, sum = 0, speed = 0, fd;
  unsigned char *msg;
  off_t file_size; //em bytes
  off_t index = 0;
  struct timespec clock_start, clock_end;

  if ( (argc < 2) ||
       ((strcmp("/dev/ttyS0", argv[1])!=0) &&
        (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

  (void) signal(SIGALRM, alarm_handler);  // instala  rotina que atende interrupcao

  msg = open_file((unsigned char *)argv[2], &file_size);

  clock_gettime(CLOCK_REALTIME, &clock_start); //start clock

  if (!llopen(fd, 0))
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
    unsigned char *header_msg = headerAL(packet, file_size, &header_size);
    
    if (!llwrite(fd, header_msg, header_size))
    {
      printf("Reached alarms limit!\n");
      return -1;
    }
  }

	unsigned char *end = control_package(C2_end, file_size, file_name, size_file_name, &size_control_package);

  llwrite(fd, end, size_control_package);
  printf("Sent trama END...\n");

  llclose(fd);

  clock_gettime(CLOCK_REALTIME, &clock_end); //stop clock

  double total_time = (clock_end.tv_sec - clock_start.tv_sec) + (clock_end.tv_nsec - clock_start.tv_nsec) / 1E9;

  printf("Time passed (in seconds): %f\n", total_time);

  sleep(2);

  close(fd);
  
  return 0;
}
