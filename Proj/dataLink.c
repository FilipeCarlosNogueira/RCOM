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

#include "utils.h"
#include "dataLink.h"

struct termios oldtio, newtio;
int alarm_counter = 0;
bool alarm_flag = TRUE;

int test = 0;

int trama = 0;
int total_tramas = 0;

/*
* Alarm handler funtion
*/
void alarm_handler(){
	alarm_flag=TRUE;
	++alarm_counter;
    printf("alarme # %d\n", alarm_counter);
}

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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
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

    printf("New termios structure set\n");
}

void send_SET(int fd, unsigned char c)
{
    unsigned char set[5];

    set[0] = FLAG;
    set[1] = A;
    set[2] = c;
    set[3] = set[1] ^ set[2];
    set[4] = FLAG;

    printf("Sendind message!\n");
    write(fd, set, 5);

    printf("Message sent: %x, %x, %x, %x, %x\n", set[0], set[1], set[2], set[3], set[4]);
}

bool read_SET(int fd, unsigned char c_set){
    
    int state=0;
    char c = 0;
    char message[5];

    while (state != 5) {       /* loop for input */

        read(fd,&c, 1);   /* reads one character at a time */

        switch(state){
            case 0:
                if(c==FLAG){
                    message[state] = c;
                    state++;
                }
                else return false;
            break;

            case 1:
                if(c==A){
                    message[state] = c;
                    state++;
                }
                else if(c == FLAG) state = 1;
                else state = 0;
            break;

            case 2:
                if(c == c_set){
                    message[state] = c;
                    state++;
                }
                else if(c == FLAG){
                    state--;
                }
                else state = 0;
            break;

            case 3:
                if(c == (A ^ c_set)){
                    message[state] = c;
                    state++;
                }
                else if(c == FLAG) state = 1;
                else state = 0;
            break;

            case 4:
                if(c == FLAG){
                    message[state] = c;
                    state++;
                }
                else state = 0;
            break;

            default:
                return false;
            break;
        }
    }

    // Writes message received
    printf("Message received: %x, %x, %x, %x, %x\n", message[0], message[1], message[2], message[3], message[4]);

    return true;
}

int llopen(int fd, int mode){

    // Transmitter
    if(mode){
       while(alarm_counter < 3) {
            if (alarm_flag) {
                //SET and writes to receptor
                send_SET(fd, C);

                printf("olaa\n");
                alarm(TIMEOUT);
                alarm_flag = FALSE;
            }

            //reads response from Receptor
            if(read_SET(fd, C_UA)) break;
	    }

        if(alarm_counter == 3) return 0;
    }
    // Receiver 
    else {
        bool result;
        do{
            result = read_SET(fd, C);
        }while(!result);
        
        printf("SET received!\n");
        send_SET(fd, C_UA);
        printf("UA sent!\n");
    }

    return 1;
}

int llclose(int fd, int mode){

    unsigned char c;

    // Transmitter
    if(mode){
        send_SET(fd, DISC);
        printf("DISC sent...\n");

        c = read_SET_W(fd);

        while (c != DISC)
        {
            c = read_SET_W(fd);
        }

        printf("DISC read...\n");

        send_SET(fd, C_UA);
        printf("Final UA sent!\n");
        printf("Emitter terminated!\n");
    }
    // Receiver
    else{
        read_SET(fd, DISC);
        printf("DISC received...\n");

        send_SET(fd, DISC);
        printf("Sent DISC...\n");

        read_SET(fd, C_UA);
        printf("UA received...\n");

        printf("Receiver terminated!\n");
    }

	if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    return 1;
}

// WRITE
unsigned char read_SET_W(int fd)
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
  unsigned char *stuffed_BCC2 = NULL;
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
    unsigned char c = read_SET_W(fd);
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


// READ
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
