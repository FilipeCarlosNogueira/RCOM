/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define C 0x03
#define UA 0x07

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

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
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



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

	//SET AND WRITE

	unsigned char set[5];

	set[0] = FLAG;
	set[1] = A;
 	set[2] = C;
	set[3] = A ^ C;
	set[4] = FLAG;

	write(fd, set, 5);

	//Response

	sleep(1);

  unsigned char resp[5];
	unsigned char c;
	char res[5];

	//reads response from other machine

  //state machine

  int state = 0;

  while(state != 5){
    if(read(fd, &c, 1) < 0){
  		perror("read failed!");
  		exit(-1);
  	}

  	switch(state) {
  		case(0):
  				if(c == FLAG){
              res[state] = c;
              state++;
          }
          break;
      case(1):
          if(c == A){
              res[state] = c;
              state++;
          }
          else state = 0;
          break;
      case(2):
          if(c == C){
              res[state] = c;
              state++;
          }
          else state = 0;
          break;
      case(3):
          if(c == res[1]^res[2]){
              res[state] = c;
              state++;
          }
          else state = 0;
          break;
      case(4):
          if(c == FLAG){
              res[state] = c;
              state++;
          }
          else state = 0;
          break;
      }
  }

  /*
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar
    o indicado no gui�o
  */

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
