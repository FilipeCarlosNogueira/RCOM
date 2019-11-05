#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "macros.h"
#include "application.h"

unsigned char packet_counter = 0;
int total_tramas = 0;

/* --------- Write --------- */

/*
* Creates the control package start and end.
* Main structure: C (T L V)*
* @param state, file_size, *file_name, size_file_name, *size_control_package
* @return package
*/
unsigned char *control_package(unsigned char state, off_t file_size, unsigned char *file_name, int size_file_name, int *size_control_package){
	
	if(state != C2_start && state != C2_end){
		perror("Controle package not recognised!");
		exit(-1);
	}

	*size_control_package = 9 * sizeof(unsigned char) + size_file_name;

	unsigned char *control_package;
	if((control_package = (unsigned char *)malloc(*size_control_package)) == NULL){
		perror("Control package maaloc failed!");
		exit(-1);
	}

	/*
	* 32 bit number range:
	* 0 <-> (2^(32) − 1)
	*/

	control_package[0] = state;						// C = start / end
	control_package[1] = T1;						// T = 0 - size of file
	control_package[2] = L1;						// L = Size of file_size
	control_package[3] = (file_size >> 24) & 0xFF;	// V = fourth 8 bits of file_size
	control_package[4] = (file_size >> 16) & 0xFF;	// V = third 8 bits of file_size
	control_package[5] = (file_size >> 8) & 0xFF;	// V = second 8 bits of file_size  
	control_package[6] = file_size & 0xFF;			// V = first 8 bits of file_size 
	control_package[7] = T2;						// T = 1 - name of file
	control_package[8] = size_file_name;			// L = size of file_name

	// V = file name
	memcpy(&control_package[9], file_name, size_file_name);

	return control_package;
}

/*
* Creates a packet either with size equal to split_packet_size or smaller.
* @param *msg, *index, *size_split_packet, file_size
* @return *split_packet
*/
unsigned char *split_msg(unsigned char *file_data, off_t *index, int *size_split_packet, off_t file_size){

	// Checks if it's possible to build a packet with the size equal to split_packet_size.
	// If not, creates a packet with the remaining bytes
	if (*index + *size_split_packet > file_size)
		*size_split_packet = file_size - *index;

	unsigned char *split_packet;
	if((split_packet = (unsigned char *)malloc(*size_split_packet)) == NULL){
		perror("slpit_msg packet malloc failed!");
		exit(-1);
	}

	// Copies portion of the file data to the split packet
	memcpy(split_packet, file_data + *index, *size_split_packet);

	*index += *size_split_packet;

	return split_packet;
}

/*
* Adds the Packet Header to the split packet
* @param *split_packet, file_size, *application_packet_size
* @return application_packet
*/
unsigned char* header(unsigned char *split_packet, int *application_packet_size){
	
	unsigned char* application_packet;
	if((application_packet = (unsigned char*)malloc(*application_packet_size + 4)) == NULL){
		perror("header application_packet malloc failed!");
		exit(-1);
	}

	// C – Control field (value: 1 – data)
	application_packet[0] = C_header;

	/*
	* Os pacotes de dados contêm obrigatoriamente um campo (um octeto) 
	* com um número de sequência e um campo (dois octetos) que indica o 
	* tamanho do respectivo campo de dados.
	*
	* -> Este tamanho depende do tamanho máximo do campo de Informação das tramas I.
	*/

	// N – Sequence number (255 module)
	application_packet[1] = packet_counter % 255;

	// (K = 256 * L2 + L1)
	// L2 - number of octets (k) of Data field
	application_packet[2] = *application_packet_size / 256;
	// L1 - number of octets (k) of Data field
	application_packet[3] = *application_packet_size % 256;

	// P1 ... PK – Data field of package (K octets)
	memcpy(application_packet + 4, split_packet, *application_packet_size);
	*application_packet_size += 4;

	++packet_counter;
	++total_tramas;

	return application_packet;
}

/* --------- Read --------- */

/*
* Parses the name of the file.
* @param *start
* @return name
*/
unsigned char* start_filename(unsigned char* start){

	// Check T2
	if(start[7] != 1) return NULL;
	
	// Parse size_file_name
	int size_file_name = (int)start[8];
	
	unsigned char* name;
	if((name = (unsigned char*)malloc(size_file_name + 1)) == NULL){
		perror("start_filename name malloc failed!");
		exit(-1);
	}

	// Parse name
	memcpy(name, &start[9], size_file_name);

	// Add termination
	name[size_file_name] = '\0';

	return name;
}

/*
* Parses the file size.
* @param *start
* @return file size in bytes
*/
off_t start_file_size(unsigned char* start){

	off_t size = 0;
	for (size_t i = 0; i < 4; ++i)
		size |= (start[3+i] << (24 - 8*i));

	return size;
}

/*
* Validates the END controle package
* @param *start, start_size, *end, end_size
* @return 
*/
bool final_packet_check(unsigned char* start, int start_size, unsigned char* end, int end_size){
	
	int start_index = 1;
	int end_index = 1;

	// check size
	if (start_size != end_size) return FALSE;
	
	// check if END trama is the equal to START trama
	if (end[0] == C2_end){
		
		while(start_index < start_size){
			if (start[start_index] != end[end_index]) return FALSE;

			++start_index;
			++end_index;
		}
		return TRUE;
	}
	
	return FALSE;
}

/*
* Removes Package Header.
* @param *remove, remove_size, *removed_size
* @return msg_removed_header
*/
unsigned char* remove_header(unsigned char* remove, int remove_size, int* removed_size){

	unsigned char* msg_removed_header;
	if((msg_removed_header = (unsigned char*)malloc(remove_size - 4)) == NULL){
		perror("remove_header msg_removed_header malloc failed!");
		exit(-1);
	}

	memcpy(msg_removed_header, &remove[4], remove_size);

	*removed_size = remove_size - 4;

	return msg_removed_header;
}