#ifndef DATALINK_H
#define DATALINK_H

/*
* Creates SET and writes to receptor
* @param fd, c
*/
void send_SET(int fd, unsigned char c);

/*
* Function used to analise the SET, UA and DISC packages
* @param fd, c_set
* @return TRUE if no errors / FALSE if errors
*/
bool read_SET(int fd, unsigned char c_set);

/*
* Establishes the first connection. 
* Transmitter sends SET
* Receiver sends UA
* @param fd: COM1, COM2, ...; mode: TRANSMITTER / RECEIVER
* @return 1 in success; -1 in failure 
*/
int llopen(int fd, int x);

/*
* Establishes the last connection
* @param fd: COM1, COM2, ... 
* @param mode: TRANSMITTER / RECEIVER
* @return 1
*/
int llclose(int fd, int mode);

/* --------- WRITE --------- */

/*
* State machine that interprets the Supervion Trama sent by the Receiver.
* @param fd
* @return C field of received trama
*/
unsigned char read_supervision_trama(int fd);

/*
* The stuffing of the BCC occurs when the Flag or Escape byte are found. 
* @param BCC2, *size_BCC2
* @return stuffed_BCC2
*/
unsigned char *stuff_BCC2(unsigned char BCC2, int *size_BCC2);

/*
* Na geração do BCC são considerados apenas os octetos originais 
* (antes da operação de stuffing), mesmo que algum octeto 
* (incluindo o próprio BCC) tenha de ser substituído pela 
* correspondente sequência de escape.
* @param *msg, size
* @return BCC2
*/
unsigned char calc_BCC2(unsigned char *msg, int size);

/*
* Builds a Information Trama respecting the Transparence specifications
* @param *buffer, length, *size_inf_trama
* @return *inf_trama
*/
unsigned char *build_information_trama(unsigned char *buffer, int length, int *size_inf_trama);

/*
* Builds the information trama and sends it to the Receiver.
* Reads the Supervision Trama sent by the Receiver and process it.
* @param fd, *buffer, length
* @return TRUE if no errors, else FALSE 
*/
int llwrite(int fd, unsigned char *buffer, int length);

/* --------- READ --------- */

/*
* Validates the BCC2.
*  @param *packet, packet_size
* @return TRUE if well built / FALSE if not
*/
int BCC2_test(unsigned char* packet, int packet_size);

/*
* Reads and validates the information trama.
* @param fd, *packet_size
* @return *packet;
*/
unsigned char* llread(int fd, int* buffer);


#endif