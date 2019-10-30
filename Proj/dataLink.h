#ifndef DATALINK_H
#define DATALINK_H

void send_SET(int fd, unsigned char c);
bool read_SET(int fd, unsigned char c_set);

int llopen(int fd, int x);
int llclose(int fd, int mode);

// WRITE

unsigned char read_supervision_trama(int fd);
unsigned char calc_BCC2(unsigned char *msg, int size);
unsigned char *stuff_BCC2(unsigned char BCC2, int *size_BCC2);
unsigned char *build_information_trama(unsigned char *buffer, int length, int *size_inf_trama);
int llwrite(int fd, unsigned char *buffer, int length);

// READ

int BCC2_test(unsigned char* msg, int msg_size);
unsigned char* llread(int fd, int* msg_size);


#endif