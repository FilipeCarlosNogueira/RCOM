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

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define C 0x03

volatile int STOP=FALSE;

int fd;
char res;
char message[5];
unsigned char UA[5];

int stateMachine(char aux, int state){
  switch(state){
      case 0:
          if(aux==FLAG){
            message[state] = aux;
            state++;
            return state;
          }
          else return state;
      break;

      case 1:
          if(aux==A){
            message[state] = aux;
            state++;
            return state;
          }
          else return state;
      break;

      case 2:
          if(aux==C){
            message[state] = aux;
            state++;
            return state;
          }
          else if(aux==FLAG){
              state--;
              return state;
          }
      break;

      case 3:
          if(aux==UA[3]){
            message[state] = aux;
            state++;
            return state;
          }
          else return state;
      break;

      case 4:
          if(aux==FLAG){
            message[state] = aux;
            state++;
            return state;
          }
          else return state;
      break;

      case 5:
          return state;
      break;

      default:
        return state;
      break;
  }
}

void getMessage(){
  int state=0;
  bool condition=false;
  char aux;

  while (!condition) {       /* loop for input */

    res = read(fd,&aux, 1);   /* reads one character at a time */

    state = stateMachine(aux, state);
    if(state == 5) condition=true;
  }
}

int main(int argc, char** argv)
{
    struct termios oldtio,newtio;
    char buf[255];

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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */


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

    //get message from emissor
    getMessage();

    // Writes message received
    printf("Message: %x, %x, %x, %x, %x\n", message[0], message[1], message[2], message[3], message[4]);

    //UA array
    UA[0]=FLAG;
    UA[1]=A;
    UA[2]=C;
    UA[3]=UA[1] ^ UA[2];
    UA[4]= FLAG;

    // Send response
    tcflush(fd, TCIOFLUSH);
    sleep(1);
    write(fd, UA, 5);

  /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o
  */

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
