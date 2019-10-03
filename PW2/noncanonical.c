/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define A 0x03
#define FLAG 0x7e
#define UA 0x07
#define CSET 0x03


volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
	char aux;
    bool condition=false;

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


    unsigned char CUA[5];
    CUA[0]=FLAG;
    CUA[1]=A;
    CUA[2]=UA;
    CUA[3]=CUA[1] ^ CUA[2];


    
    int state=0;
	while (!condition) {       /* loop for input */
        res = read(fd,&aux, 1);   /* reads one character at a time */
        printf("%X\n", aux);
        
        
        
        switch(state){
            case 1:            
                if(aux==FLAG){
                    state++;
                    continue;
                }
                else{
                    continue;
                }   
            
            
            
            break;

            case 2:
                if(aux==A){
                    state++;
                    continue;
                }
                else{
                    continue;
                }
                

            break;
            
            case 3:
                if(aux==CSET){
                    state++;
                    continue;
                }
                else if(aux==FLAG){
                    state--;
                    continue;
                }
            
            break;
            
            case 4:
                if(aux==CUA[3]){
                    state++;
                    continue;
                }

                else{
                    continue;
                }
                            
            break;

            case 5:
                if(aux==FLAG){
                    state++;
                    continue;
                }

                else
                {
                    continue;
                }
                
            break;


            case 6:
                condition=true;
            break;
            
            default:
            continue;
            break;
        }
      
      
		
    }


  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
