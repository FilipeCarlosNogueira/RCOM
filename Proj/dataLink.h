#ifndef DATALINK_H
#define DATALINK_H

#include <stdbool.h>

extern struct termios oldtio, newtio;
extern int alarm_counter;
extern bool alarm_flag;

extern int total_tramas;

void alarm_handler();
void setTermios(int fd);

void send_SET(int fd, unsigned char c);
bool read_SET(int fd, unsigned char c_set);

int llopen(int fd, int x);
int llclose(int fd, int mode);

// WRITE

unsigned char read_SET_W(int fd);
unsigned char *change_BCC1(unsigned char *packet, int size_packet);
unsigned char *change_BCC2(unsigned char *packet, int size_packet);
unsigned char calc_BCC2(unsigned char *msg, int size);
unsigned char *stuff_BCC2(unsigned char BCC2, int *size_BCC2);
int llwrite(int fd, unsigned char *msg, int size);

// READ

int BCC2_test(unsigned char* msg, int msg_size);
unsigned char* llread(int fd, int* msg_size);


#endif