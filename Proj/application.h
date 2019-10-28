#ifndef APPLICATION_H
#define APPLICATION_H

unsigned char msg_counter;
extern int total_tramas;

/* --------- Write --------- */
unsigned char *control_package(unsigned char state, off_t file_size, unsigned char *file_name, int size_file_name, int *size_control_package);
unsigned char *split_msg(unsigned char *msg, off_t *index, int *size_packet, off_t file_size);
unsigned char* header(unsigned char* msg, off_t file_size, int* packet_size);

/* --------- Read --------- */
unsigned char* start_filename(unsigned char* start);
off_t start_file_size(unsigned char* start);
int final_msg_check(unsigned char* start, int start_size, unsigned char* final, int final_size);
unsigned char* remove_header(unsigned char* remove, int remove_size, int* removed_size);

#endif