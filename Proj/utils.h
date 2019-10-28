#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

extern struct termios oldtio, newtio;
extern int alarm_counter;
extern bool alarm_flag;

void alarm_handler();
void setTermios(int fd);
unsigned char *open_file(unsigned char *file_name, off_t *file_size);
void new_file(unsigned char* msg, off_t* file_size, unsigned char filename[]);

#endif