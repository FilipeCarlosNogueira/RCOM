#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

extern struct termios oldtio, newtio;
extern int alarm_counter;
extern bool alarm_flag;

/*
* Opens serial port.
* Builds termios 
* @param **argv
*/
void setTermios(int fd);

/*
* Alarm handler funtion
*/
void alarm_handler();

/*
* Opens file, reads and saves its content.
* Returns everything that was saved.
* @param *file_name, *file_size
* return *file_data
*/
unsigned char *open_file(unsigned char *file_name, off_t *file_size);

/*
* Creates a new file.
* @param *msg, *file_size, filename[]
*/
void new_file(unsigned char* msg, off_t* file_size, unsigned char filename[]);

#endif