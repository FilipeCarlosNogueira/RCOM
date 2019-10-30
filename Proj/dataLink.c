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
#include "utils.h"
#include "dataLink.h"

/*
* Creates SET and writes to receptor
* @param fd, c
*/
void send_SET(int fd, unsigned char c){
  unsigned char set[5];

  set[0] = FLAG;
  set[1] = A;
  set[2] = c;
  set[3] = set[1] ^ set[2];
  set[4] = FLAG;

  write(fd, set, 5);
}

/*
* Function used to analise the SET and UA packages
* @param fd, c_set
* @return TRUE if no errors / FALSE if errors
*/
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
  //printf("Message received: %x, %x, %x, %x, %x\n", message[0], message[1], message[2], message[3], message[4]);

  return true;
}

/*
* Establishes the first connection. 
* Transmitter sends SET
* Receiver sends UA
* @param fd: COM1, COM2, ...; mode: TRANSMITTER / RECEIVER
* @return 1 in success; -1 in failure 
*/
int llopen(int fd, int mode){

  // Transmitter
  if(mode){
    while(alarm_counter < 3) {
      if (alarm_flag) {

        send_SET(fd, C);
        printf("SET send!\n");

        alarm(TIMEOUT);
        alarm_flag = FALSE;
      }

      //reads response from Receptor
      if(read_SET(fd, C_UA)){
        printf("UA received!\n\n");
        break;
      }
    }

    if(alarm_counter == 3) return -1;
  }
  // Receiver 
  else {
      bool result;
      do{
        result = read_SET(fd, C);
      }while(!result);

      printf("SET received!\n");

      send_SET(fd, C_UA);
      printf("UA sent!\n\n");
  }

  return 1;
}

/*
* Establishes the last connection
* Transmitter sends 
* 
* @param fd: COM1, COM2, ... 
* @param mode: TRANSMITTER / RECEIVER
*/
int llclose(int fd, int mode){

  unsigned char c;

  // Transmitter
  if(mode){
    send_SET(fd, DISC);
    printf("DISC sent...\n");

    c = read_SET_W(fd);

    while (c != DISC)
      c = read_SET_W(fd);

    printf("DISC read...\n");

    send_SET(fd, C_UA);
    printf("Final UA sent!\n");
  }
  // Receiver
  else{
    read_SET(fd, DISC);
    printf("DISC received...\n");

    send_SET(fd, DISC);
    printf("Sent DISC...\n");

    read_SET(fd, C_UA);
    printf("UA received...\n");
  }

	if (tcsetattr(fd, TCSANOW, &oldtio) == -1){
    perror("tcsetattr");
    exit(-1);
  }

  return 1;
}

/* --------- WRITE --------- */

/*
* State machine that interprets the Supervion Trama sent by the Receiver.
* @param fd
* @return C field of received trama
*/
unsigned char read_supervision_trama(int fd){
  
  int state = 0;
  unsigned char c;
  unsigned char c1;

  while (!alarm_flag && state != 5){
    
    read(fd, &c, 1);
    
    switch (state){
      case 0:
        if (c == FLAG)
          state = 1;
      break;
      
      case 1:
        if (c == A)
          state = 2;
        else if (c == FLAG)
          state = 1;
        else
          state = 0;
      break;

      case 2:
        if (c == C_RR0 || c == C_RR1 || c == C_REJ0 || c == C_REJ1 || c == DISC){
          c1 = c;
          state = 3;
        }
        else if (c == FLAG)
          state = 1;
        else
          state = 0;
      break;

      case 3:
        if (c == (A ^ c1))
          state = 4;
        else
          state = 0;
      break;

      case 4:
        if (c == FLAG){
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

/*
* The stuffing of the BCC occurs when the Flag or Escape byte are found. 
* @param BCC2, *size_BCC2
* @return stuffed_BCC2
*/
unsigned char *stuff_BCC2(unsigned char BCC2, int *size_BCC2){

  unsigned char *stuffed_BCC2 = NULL;
  
  /*
  * Se no interior da trama ocorrer o octeto 01111110 (0x7e), 
  * isto é, o padrão que corresponde a uma flag, o octeto é 
  * substituído pela sequência 0x7d 0x5e (octeto de escape 
  * seguido do resultado do ou exclusivo do octeto substituído 
  * com o octeto0x20).
  */
  if (BCC2 == FLAG){
    if((stuffed_BCC2 = (unsigned char *)malloc(2 * sizeof(unsigned char *))) == NULL){
      perror("FLAG stuffed_BCC2 malloc failed!");
      exit(-1);
    }
    stuffed_BCC2[0] = esc;
    stuffed_BCC2[1] = esc_flag;
    (*size_BCC2)++;
  }

  /*
  * Se no interior da trama ocorrer o octeto 01111101 (0x7d), 
  * isto é, o padrão que corresponde ao octeto de escape, o 
  * octeto é substituído pela sequência 0x7d 0x5d (octeto de 
  * escape seguido do resultado do ou exclusivo do octeto 
  * substituído com o octeto 0x20).
  */
  else if (BCC2 == esc){
    if((stuffed_BCC2 = (unsigned char *)malloc(2 * sizeof(unsigned char *))) == NULL){
      perror("ESC stuffed_BCC2 malloc failed!");
      exit(-1);
    }
    stuffed_BCC2[0] = esc;
    stuffed_BCC2[1] = esc_escape;
    (*size_BCC2)++;
  }

  return stuffed_BCC2;
}

/*
* Na geração do BCC são considerados apenas os octetos originais 
* (antes da operação de stuffing), mesmo que algum octeto 
* (incluindo o próprio BCC) tenha de ser substituído pela 
* correspondente sequência de escape.
* @param *msg, size
* @return BCC2
*/
unsigned char calc_BCC2(unsigned char *msg, int size){

  unsigned char BCC2 = msg[0];

  for (int i = 1; i < size; ++i)
    BCC2 ^= msg[i];

  return BCC2;
}

int trama = 0;
/*
* Builds a Information Trama respecting the Transparence specifications
* @param *buffer, length, *size_inf_trama
* @return *inf_trama
*/
unsigned char *build_information_trama(unsigned char *buffer, int length, int *size_inf_trama){
  
  unsigned char *inf_trama;
  if((inf_trama = (unsigned char *)malloc((length + 6) * sizeof(unsigned char))) == NULL){
    perror("llwrite inf_trama malloc failed!");
    exit(-1);
  }
  *size_inf_trama = length + 6;

  // F - FLAG
  inf_trama[0] = FLAG;

  // A - Address field
  inf_trama[1] = A;

  // C - Control field
  if (trama == 0)
    inf_trama[2] = C10;
  else
    inf_trama[2] = C11;

  // BCC1 - Protection field - Head
  inf_trama[3] = (inf_trama[1] ^ inf_trama[2]);

  // D1 + Data + Dn
  int i = 0;
  int j = 4;
  while(i < length){

    if (buffer[i] == FLAG || buffer[i] == esc){
      
      if((inf_trama = (unsigned char *)realloc(inf_trama, ++*size_inf_trama)) == NULL){
        perror("llwrite inf_trama realloc failed!");
        exit(-1);
      }
      
      inf_trama[j] = esc;
      
      /*
      * Se no interior da trama ocorrer o octeto 01111110 (0x7e), 
      * isto é, o padrão que corresponde a uma flag, o octeto é 
      * substituído pela sequência 0x7d 0x5e (octeto de escape 
      * seguido do resultado do ou exclusivo do octeto substituído 
      * com o octeto0x20).
      */
      if(buffer[i] == FLAG) 
        inf_trama[j + 1] = esc_flag;

      /*
      * Se no interior da trama ocorrer o octeto 01111101 (0x7d), 
      * isto é, o padrão que corresponde ao octeto de escape, o 
      * octeto é substituído pela sequência 0x7d 0x5d (octeto de 
      * escape seguido do resultado do ou exclusivo do octeto 
      * substituído com o octeto 0x20).
      */
      if(buffer[i] == esc) 
        inf_trama[j + 1] = esc_escape;
      
      j += 2;
    }
    else{
      inf_trama[j] = buffer[i];
      ++j;
    }

    ++i;
  }

  // BCC2 - Protection field - Data
  unsigned char BCC2 = calc_BCC2(buffer, length);
  int size_BCC2 = 1;

  unsigned char *BCC2_stuffed;
  if((BCC2_stuffed = (unsigned char *)malloc(sizeof(unsigned char))) == NULL){
    perror("llwrite BCC2_stuffed malloc failed!");
    exit(-1);
  }
  BCC2_stuffed = stuff_BCC2(BCC2, &size_BCC2);

  if (size_BCC2 == 1) /* If no Flag or Escape byte is found in BCC2 */
    inf_trama[j] = BCC2;
  else{
    if((inf_trama = (unsigned char *)realloc(inf_trama, ++*size_inf_trama)) == NULL){
      perror("llwrite inf_trama BCC2 realloc failed!");
      exit(-1);
    }
    inf_trama[j] = BCC2_stuffed[0];
    inf_trama[j + 1] = BCC2_stuffed[1];
    j++;
  }

  // F - FLAG
  inf_trama[j + 1] = FLAG;

  return inf_trama;
}

/*
* Builds the information trama and sends it to the Receiver.
* Reads the Supervision Trama sent by the Receiver and process it.
* @param fd, *buffer, length
* @return TRUE if no errors, else FALSE 
*/
int llwrite(int fd, unsigned char *buffer, int length){

  int size_inf_trama = 0;
  unsigned char *inf_trama = build_information_trama(buffer, length, &size_inf_trama);

  int flag = FALSE;
  unsigned char c;
  while(1){

    write(fd, inf_trama, size_inf_trama);

    alarm_flag = FALSE;
    
    alarm(TIMEOUT);
    
    c = read_supervision_trama(fd);

    // If the Receiver validates the Information Trama, llwrite was successful
    if ((c == C_RR1 && trama == 0) || (c == C_RR0 && trama == 1)){
      printf("RR received! Value: %x. Trama = %d\n", c, trama);
      flag = FALSE;
      alarm_counter = 0;
      trama ^= 1;
      alarm(0);
    }
    
    // If an error occurred, the Receiver will send a REJ trama,
    // and a retransmission will occur.
    else if (c == C_REJ1 || c == C_REJ0){
      flag = TRUE;
      printf("REJ received! Value: %x. Trama = %d\n", c, trama);
      alarm(0);
    }

    if(!alarm_flag && (alarm_counter >= MAX)) break;
    if(!flag) break;
  }

  if (alarm_counter >= MAX)
    return FALSE;
  
  return TRUE;
}


/* --------- READ --------- */

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
	
  return FALSE;
}

int test = 0;
unsigned char* llread(int fd, int* msg_size)
{
	unsigned char* msg = (unsigned char*)malloc(0);
	*msg_size = 0;
	unsigned char read_c;
	unsigned char c;
	int trama = 0;
	int send_data = FALSE;
	int state = 0;

	while (state != 6){

		read(fd, &c, 1);

		switch (state){
      case 0:
        if (c == FLAG)
          state = 1;
      break;

      case 1:
        if (c == A)
          state = 2;
        else if (c == FLAG)
          state = 1;
        else
          state = 0;
      break;

      case 2:
        if (c == C10){
          trama = 0;
          read_c = c;
          state = 3;
        }
        else if (c == C11){
          trama = 1;
          read_c = c;
          state = 3;
        }
        else{
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
        if (c == FLAG){
          
          if (BCC2_test(msg, *msg_size)){
            
            if (trama == 0)
              send_SET(fd, C_RR1);
            else
              send_SET(fd, C_RR0);

            state = 6;
            send_data = TRUE;
            printf("RR sent, trama -> %d\n", trama);
          }

          else{
            
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
          state = 5;
        else{
          msg = (unsigned char*)realloc(msg, ++(*msg_size));
          msg[*msg_size - 1] = c;
        }
      break;

      case 5:
        if (c == esc_flag){
          msg = (unsigned char*)realloc(msg, ++(*msg_size));
          msg[*msg_size - 1] = FLAG;
        }
        else if (c == esc_escape){
          msg = (unsigned char*)realloc(msg, ++(*msg_size));
          msg[*msg_size - 1] = esc;
        }
        else{
          perror("Invalid character after esc character");
          exit(-1);
        }
        state = 4;
      break;
		}
	}

	printf("Message size = %d\n", *msg_size);

	if((msg = (unsigned char*)realloc(msg, *msg_size - 1)) == NULL){
    perror("Message realloc failed!");
    exit(-1);
  }
	*msg_size = *msg_size - 1;

	if (send_data){
		if (trama == test)
			test ^= 1;
		else
			*msg_size = 0;
	}
	else
		*msg_size = 0;
  
	return msg;
}
