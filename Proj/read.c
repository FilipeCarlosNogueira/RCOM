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
struct termios oldtio,newtio;

char res;
char message[5];
unsigned char UA[5];

/*
* Emissor message state machine
* @param aux, state
* return state
*/
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
  return -1;
}

/*
* While loop to get message from Emissor
*/
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

/*
* Opens serial port.
* Builds termios 
* @param **argv
*/
void setTermios(char **argv){
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
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */


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

/* ----------------- Data link Layer ----------------- */

/*
*
*/
bool llopen(){
    //get message from emissor
    sleep(1);
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

    /*
    // Auxiliar sleep
    printf("sleeping\n");
    sleep(4);
    printf("not sleeping\n");
    */

    // Write response to Emissor
    write(fd, UA, 5);
    printf("Response: %x, %x, %x, %x, %x\n", UA[0], UA[1], UA[2], UA[3], UA[4]);

    return true;
}

/*
*
*/
bool llclose(){
    tcsetattr(fd,TCSANOW,&oldtio);

    return true;
}

/* ----------------- Main ----------------- */
int main(int argc, char** argv)
{
    char buf[255];

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
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
        }
    #endif

    setTermios(argv);

    // Establecimento
    llopen();

    // Transferência de dados

    // Terminação
    llclose();

    close(fd);
    return 0;
}
