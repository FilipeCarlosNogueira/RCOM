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

unsigned char msg_counter = 0;
int total_tramas = 0;

/* --------- Write --------- */

/*
*
*/
unsigned char *control_package(unsigned char state, off_t file_size, unsigned char *file_name, int size_file_name, int *size_control_package)
{
	if(state != C2_start && state != C2_end){
		perror("Controle package not recognised!");
		exit(-1);
	}

	*size_control_package = 9 * sizeof(unsigned char) + size_file_name;
	printf("%lu --\n", sizeof(unsigned char));

	unsigned char *package;
	if((package = (unsigned char *)malloc(*size_control_package)) == NULL){
	perror("Control package maaloc failed!");
	exit(-1);
	}

	package[0] = state;
	package[1] = T1;
	package[2] = L1;
	package[3] = (file_size >> 24) & 0xFF;
	package[4] = (file_size >> 16) & 0xFF;
	package[5] = (file_size >> 8) & 0xFF;
	package[6] = file_size & 0xFF;
	package[7] = T2;
	package[8] = size_file_name;

	int i = 0;
	while(i < size_file_name)
	{
	package[9 + i] = file_name[i];
	i++;
	}
	return package;
}

unsigned char *split_msg(unsigned char *msg, off_t *index, int *size_packet, off_t file_size)
{
  unsigned char *packet;
  int i = 0;
  off_t j = *index;

  if (*index + *size_packet > file_size)
    *size_packet = file_size - *index;

  packet = (unsigned char *)malloc(*size_packet);

  while(i < *size_packet)
  {
    packet[i] = msg[j];
    i++;
    j++;
  }

  *index = j;
  
  return packet;
}

unsigned char* header(unsigned char* msg, off_t file_size, int* packet_size)
{
	unsigned char* final_msg = (unsigned char*)malloc(file_size + 4);

	final_msg[0] = C_header;
	final_msg[1] = msg_counter % 255;
	final_msg[2] = (int)file_size / 256;
	final_msg[3] = (int)file_size % 256;

	memcpy(final_msg + 4, msg, *packet_size);
	*packet_size += 4;

	msg_counter++;
	total_tramas++;

	return final_msg;
}

/* --------- Read --------- */

unsigned char* start_filename(unsigned char* start)
{
	int auxL2 = (int)start[8];
	unsigned char* name = (unsigned char*)malloc(auxL2 + 1);

	int i;

	for (i = 0; i < auxL2; i++)
	{
		name[i] = start[9 + i];
	}

	name[auxL2] = '\0';

	return name;
}

off_t start_file_size(unsigned char* start)
{
	return (start[3] << 24) | (start[4] << 16) | (start[5] << 8) | (start[6]);
}

int final_msg_check(unsigned char* start, int start_size, unsigned char* final, int final_size)
{
	int s = 1;
	int f = 1;

	if (start_size != final_size)
		return FALSE;
	else
	{
		if (final[0] == C2_end)
		{
			while(s < start_size)
			{
				if (start[s] != final[f])
					return FALSE;

				s++;
				f++;
			}
			return TRUE;
		}
		else
			return FALSE;
	}
}

unsigned char* remove_header(unsigned char* remove, int remove_size, int* removed_size)
{
	int i = 0;
	int j = 4;

	unsigned char* msg_removed_header = (unsigned char*)malloc(remove_size - 4);

	while(i < remove_size)
	{
		msg_removed_header[i] = remove[j];

		i++;
		j++;
	}

	*removed_size = remove_size - 4;

	return msg_removed_header;
}