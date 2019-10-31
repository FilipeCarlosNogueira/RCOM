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
* Function used to analise the SET, UA and DISC packages
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

  switch (mode){
    case TRANSMITTER:
      
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
    break;

    bool result;
    case RECEIVER:
      
      do{
        result = read_SET(fd, C);
      }while(!result);

      printf("SET received!\n");

      send_SET(fd, C_UA);
      printf("UA sent!\n\n");
    break;
    
    default:
    break;
  }

  return 1;
}

/*
* Establishes the last connection
* @param fd: COM1, COM2, ... 
* @param mode: TRANSMITTER / RECEIVER
* @return 1
*/
int llclose(int fd, int mode){

  switch (mode){
    bool result;
    case TRANSMITTER:
      send_SET(fd, DISC);
      printf("DISC sent...\n");

      do{
        result = read_SET(fd, DISC);
      } while (!result);
      printf("DISC read...\n");

      send_SET(fd, C_UA);
      printf("Final UA sent!\n");
    break;
    
    case RECEIVER:
      read_SET(fd, DISC);
      printf("DISC received...\n");

      send_SET(fd, DISC);
      printf("Sent DISC...\n");

      read_SET(fd, C_UA);
      printf("UA received...\n");
    break;
    
    default:
    break;
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
  
  if (BCC2 == FLAG || BCC2 == esc){

    if((stuffed_BCC2 = (unsigned char *)malloc(2 * sizeof(unsigned char *))) == NULL){
      perror("stuffed_BCC2 malloc failed!");
      exit(-1);
    }
    stuffed_BCC2[0] = esc;
    
    if(BCC2 == FLAG) stuffed_BCC2[1] = esc_flag;

    if(BCC2 == FLAG) stuffed_BCC2[1] = esc_escape;

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

int ns = 0;
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
  if (ns == 0)
    inf_trama[2] = C10;
  else
    inf_trama[2] = C11;

  // BCC1 - Protection field - Head
  inf_trama[3] = (inf_trama[1] ^ inf_trama[2]);

  // D1 + ... + Dn
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

  unsigned char *BCC2_stuffed = stuff_BCC2(BCC2, &size_BCC2);

  if (BCC2_stuffed == NULL) /* If no Flag or Escape byte is found in BCC2 */
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
    printf("I sent! Ns = %d\n", ns);

    alarm_flag = FALSE;
    
    alarm(TIMEOUT);
    
    c = read_supervision_trama(fd);

    // If the Receiver validates the Information Trama, llwrite was successful
    if ((c == C_RR1 && ns == 0) || (c == C_RR0 && ns == 1)){
      flag = FALSE;
      alarm_counter = 0;
      ns ^= 1;
      alarm(0);
      printf("RR received! Value: %x. Nr = %d\n", c, ns);
    }
    
    // If an error occurred, the Receiver will send a REJ trama,
    // and a retransmission will occur.
    else if (c == C_REJ1 || c == C_REJ0){
      flag = TRUE;
      printf("REJ received! Value: %x. Nr = %d\n", c, ns);
      alarm(0);
      continue;
    }

    if(!alarm_flag && (alarm_counter >= MAX)) break;
    if(!flag) break;
  }

  if (alarm_counter >= MAX)
    return FALSE;
  
  return TRUE;
}


/* --------- READ --------- */

/*
* Validates the BCC2.
*  @param *packet, packet_size
* @return TRUE if well built / FALSE if not
*/
int BCC2_test(unsigned char* packet, int packet_size){

	int i = 1;
	unsigned char BCC2 = packet[0];

	while(i < packet_size - 1){
		BCC2 ^= packet[i];
		i++;
	}

	if (BCC2 == packet[packet_size - 1])
		return TRUE;
	
  return FALSE;
}

/*
* Reads and validates the information trama.
* @param fd, *packet_size
* @return *packet;
*/
int nr = 0;
unsigned char* llread(int fd, int* packet_size){

  *packet_size = 0;
	bool send_data = FALSE;

  unsigned char* packet;
  if((packet = (unsigned char*)malloc(0)) == NULL){
    perror("read_trama_I packet malloc failed!");
    exit(-1);
  }

  int state = 0;
  unsigned char control_byte;
	unsigned char c;
  
  // Read and interpret the Information Trama 
  while (state != 6){

		read(fd, &c, 1);

		switch (state){
      // FLAG
      case 0:
        if (c == FLAG)
          state = 1;
      break;

      // A
      case 1:
        if (c == A)
          state = 2;
        else
          state = 0;
      break;

      // C
      case 2:
        if (c == C10){
          nr = 1;
          control_byte = c;
          state = 3;
        }
        else if (c == C11){
          nr = 0;
          control_byte = c;
          state = 3;
        }
        else if (c == FLAG)
          state = 1;
        else
          state = 0;
      break;

      // BCC1
      case 3:
        if (c == (A ^ control_byte))
          state = 4;
        else
          state = 0;
      break;

      // Package Data + ESCAPE + FLAG
      case 4:

        // FLAG
        if (c == FLAG){
          
          /*
          * Tramas I recebidas sem erros detectados no cabeçalho e 
          * no campo de dados são aceites para processamento:
          * 
          * Se se tratar duma nova trama, o campo de dados é aceite 
          * (e passado à Aplicação), e a trama deve ser confirmada 
          * com RR.
          * 
          * Se se tratar dum duplicado, o campo de dados é descartado, 
          * mas deve fazer-se confirmação da trama com RR.
          */
          if (BCC2_test(packet, *packet_size)){
            
            if (nr)
              send_SET(fd, C_RR1);
            else
              send_SET(fd, C_RR0);
              

            state = 6;
            send_data = TRUE;
            printf("RR sent! Nr = %d\n", nr);
          }

          /*
          * Tramas I sem erro detectado no cabeçalho mas com erro 
          * detectado (pelo respectivo BCC) no campo de dados :
          * 
          * Se se tratar duma nova trama, é conveniente fazer um pedido 
          * de retransmissão com REJ, o que permite antecipar a 
          * ocorrência de time-out no emissor. 
          */
          else{
            
            if (nr)
              send_SET(fd, C_REJ1);
            else
              send_SET(fd, C_REJ0);

            state = 6;
            send_data = FALSE;
            printf("REJ sent! Nr = %d\n", nr);
          }
        }
        
        // ESCAPE
        else if (c == esc) state = 5;

        // Package Data
        else{
          if((packet = (unsigned char*)realloc(packet, ++(*packet_size))) == NULL){
            perror("read_trama_I case 4 packet realloc failed!");
            exit(-1);
          }
          packet[*packet_size - 1] = c;
        }
      break;

      // Associated ESCAPE flags
      case 5:

        // ESCAPE FLAG
        if (c == esc_flag){
          if((packet = (unsigned char*)realloc(packet, ++(*packet_size))) == NULL){
            perror("read_trama_I esc_flag packet realloc failed!");
            exit(-1);
          }
          packet[*packet_size - 1] = FLAG;
        }

        // ESCAPE ESCAPE
        else if (c == esc_escape){
          if((packet = (unsigned char*)realloc(packet, ++(*packet_size))) == NULL){
            perror("read_trama_I esc_escape packet realloc failed!");
            exit(-1);
          }
          packet[*packet_size - 1] = esc;
        }

        else{
          perror("Invalid character after esc character");
          exit(-1);
        }

        state = 4;
      break;
		}
	}

  if((packet = (unsigned char*)realloc(packet, *packet_size - 1)) == NULL){
    perror("Message realloc failed!");
    exit(-1);
  }
	*packet_size -= 1;

	printf("Message size = %d\n", *packet_size);

	// if (send_data && Ns == nr)
	// 	nr ^= 1;
	// else
	// 	*packet_size = 0;
  if(!send_data) *packet_size = 0;
  
	return packet;
}
