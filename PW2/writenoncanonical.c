/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7e
#define A 0x03
#define C 0x03

int fd;
unsigned char set[5];
unsigned char response[5];

int alarmFlag = 1;
int alarmCounter = 0;

/*
* Response state machine
* @param c, state
* return state
*/
int stateMachine(char c, int state){
    switch(state) {
        case(0):
            if(c == FLAG){
                response[state] = c;
                state++;
            }
            return state;
        break;
        case(1):
            if(c == A){
                response[state] = c;
                state++;
            }
            return state;
        break;
        case(2):
            if(c == C){
                response[state] = c;
                state++;
            }
            else if(c == FLAG) state--;

            return state;
        break;
        case(3):
            if(c == set[3]){
                response[state] = c;
                state++;
            }
            return state;
        break;
        case(4):
            if(c == FLAG){
                response[state] = c;
                state++;
            }
            return state;
        break;
        default:
          return state;
        break;
    }

    return -1;
}

/*
* While loop to get response from Receptor
* @param 
* return bool
*/
bool getResponse(){
  int state = 0;
  char c;

  while(state != 5){

    int bitReceived = 0;
    if((bitReceived = read(fd, &c, 1)) < 0){
        perror("read failed!");
        exit(-1);
    } else if (bitReceived == 0) return false;

    state = stateMachine(c, state);
  }

  return true;
}

/*
* Alarm handler funtion
*/
void timeOutHandler(){
	++alarmCounter;
	alarmFlag = 1;
	printf("Timeout nº: %d!\n", alarmCounter);
}

/*
* Set and Write
* Creates SET array and sends it to Receptor
*/
void setWrite(){

	set[0] = FLAG;
	set[1] = A;
 	set[2] = C;
	set[3] = A ^ C;
	set[4] = FLAG;

	write(fd, set, 5);
}

int main(int argc, char** argv)
{
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
  	    //  ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	    //   (strcmp("/dev/ttyS1", argv[1])!=0) )) {
          ((strcmp("/tmp/rcom0", argv[1])!=0) &&
  	      (strcmp("/tmp/rcom1", argv[1])!=0) )) {
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

    // Alarm handler
    (void) signal(SIGALRM, timeOutHandler);

	while(alarmCounter < 3) {
		if (alarmFlag) {
			//SET and writes to receptor
            printf("Sendind message!\n");
            setWrite();
            printf("Message sent: %x, %x, %x, %x, %x\n", set[0], set[1], set[2], set[3], set[4]);

            alarm(3);
			alarmFlag = 0;
        }

        //reads response from Receptor
        if (getResponse()) {
            printf("Response: %x, %x, %x, %x, %x\n", response[0], response[1], response[2], response[3], response[4]);
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
