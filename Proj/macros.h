#ifndef MACROS_H
#define MACROS_H

#include <stdlib.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TRANSMITTER 1
#define RECEIVER 0 

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

#define split_packet_size 100

#define C_header 0x01

#endif