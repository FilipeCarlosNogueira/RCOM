#ifndef APPLICATION_H
#define APPLICATION_H

unsigned char packet_counter;
extern int total_tramas;

/* --------- Write --------- */

/*
* Creates the control package start and end.
* Main structure: C (T L V)*
* @param state, file_size, *file_name, size_file_name, *size_control_package
* @return package
*/
unsigned char *control_package(unsigned char state, off_t file_size, unsigned char *file_name, int size_file_name, int *size_control_package);

/*
* Creates a packet either with size equal to split_packet_size or smaller.
* @param *msg, *index, *size_split_packet, file_size
* @return *split_packet
*/
unsigned char *split_msg(unsigned char *file_data, off_t *index, int *size_split_packet, off_t file_size);

/*
* Adds the Packet Header to the split packet
* @param *split_packet, file_size, *application_packet_size
* @return application_packet
*/
unsigned char* header(unsigned char* split_packet, int* application_packet_size);

/* --------- Read --------- */

/*
* Parses the name of the file.
* @param *start
* @return name
*/
unsigned char* start_filename(unsigned char* start);

/*
* Parses the file size.
* @param *start
* @return file size in bytes
*/
off_t start_file_size(unsigned char* start);

/*
* Validates the END controle package
* @param *start, start_size, *end, end_size
* @return 
*/
bool final_packet_check(unsigned char* start, int start_size, unsigned char* end, int end_size);

/*
* Removes Package Header.
* @param *remove, remove_size, *removed_size
* @return msg_removed_header
*/
unsigned char* remove_header(unsigned char* remove, int remove_size, int* removed_size);

#endif