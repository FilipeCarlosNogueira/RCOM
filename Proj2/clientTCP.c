#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include "clientTCP.h"
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>


size_t min(size_t a, size_t b){
	if(a<b)
		return a;
	return b;
}

/**
	* We use this function because different servers reply in different ways.
	* param buf pointer to the string to parse.
	* param fsize pointer to where the size should be written (in bytes)
	* return 1 if fsize is exact, 0 otherwise, and -1 if fails (fsize becomes 0)
  */
/**
	* replies may come in many formats - bytes, kbytes...*/
char get_filesize_150(char* buf, size_t* fsize){
		char precision=0;
		/*determine the integer (or float)
		associated with the file size*/
		char* sizeStr;
		printf("str is %s\n", buf);

		sizeStr = strstr(buf, "bytes");
		// find bytes in string.
		if(sizeStr==NULL){//didn't find
			*fsize=0;
			return -1;
		}
		sizeStr-=2; // sizeStr should start at least 2 chars before "bytes"
		while(1){
			// go back while we have numbers
			if( (('0' <=*(sizeStr-1) && *(sizeStr-1)<='9' )||*(sizeStr-1)=='.') && sizeStr-1>buf){
				sizeStr--;
			}else{
				break;
			}
		}

		printf("guessing size is near %s\n", sizeStr);

		//checking if we're talking bytes, kbytes, or something else.

		char* multipier = strchr(sizeStr, ' ');

		unsigned long mult=0;
		printf("switching on %c\n", *(multipier+1));
		switch (*(multipier+1)) {
			case 'b': // b is the first char of Bytes.
				mult=1;
				precision=1;
			break;
			case 'k': // k means kilo.
				mult=1000;
			break;
			default:
				//in case unrecognized multiplier.e  function  fread()  reads nmemb items of data, each size bytes long, from the stream pointed to by stream, storing them at the location given by ptr.

				*fsize=0;
				return -1;
			break;
		}

		*fsize=(unsigned int)(mult*atof(sizeStr)); // store fsize

		return precision;
}


int get_reply_code(char *str)
{
	int code;
	code = atoi(str);
	if (code == 421){
		//termination code.
		exit(-9);
	}
	return code;
}

size_t get_PASV_port(char* cmd){
	int p1, p2;
	char* index = strchr(cmd, '(');
	if(index==NULL){
		exit(-8);
	}

	sscanf(index, "(%*d, %*d, %*d, %*d, %d, %d)", &p1, &p2);
	return p1*256+p2;
}

long find_command_space_str(char* str, int code){
	size_t len, i;
	len=strlen(str);
	for(i = 0; i < len-3; i++){
		if(atoi(str+i)==code && str[i+3]==' ')
		return i;
	}
	return -1;
}

/**
	* We don't read directly because there is the
	* possibility that we'll get a response displayed in multiple lines.
	* param sockfd - file descriptor of the socket from which we'll read.
	* param strRet if NULL, no string is passed, else memory is allocked and string is written there
	* returns reply code
  */
int get_response(int sockfd, char** strRet){
	if(strRet!=NULL){
		(*strRet)=malloc(2048*sizeof(char));
		bzero(*strRet, 2048);
	}
	int code;
	int bytes;
	char buf[2048];
	bzero(buf, 2048);
	bytes = read(sockfd, buf, 2048);
	
	if(buf[3]=='-'){
		// multiple lines response
		code = get_reply_code(buf);
		int bytes_counter=bytes; 
		if(find_command_space_str(buf, code)>=0){
			if(strRet!=NULL){
				strncat(*strRet, buf, 2048-bytes_counter);
				bytes_counter+=bytes;
			}
			return code;
		}
		while(1){
			bytes = read(sockfd, buf, 2048);
			if(strRet!=NULL){
				strncat(*strRet, buf, 2048-bytes_counter);
				bytes_counter+=bytes;
			}
			if(find_command_space_str(buf, code)>=0)
				return code;
		}
	}
	else{
		//single line response.
		code = get_reply_code(buf);
		if(strRet!=NULL)
			strncpy(*strRet, buf, 2048); // write response to strRet if it is not NULL
	}
	return code;
}


void start(){
		printf("\033[H\033[J");
		printf("\e[?25l");
}
void finish(){
	printf("\e[?25h\n");
}

void display(unsigned int currentSize, unsigned int totalSize){
	static int barSize = 40;
  double progress = (((double)currentSize)/totalSize)*100;
  int progresSize = (int)((progress * barSize)/100);

	printf("\033[A\r[");
  int i;  
  for(i = 0; i < progresSize; i++){
    printf("%c", '*');
  }
  for(i = progresSize; i < barSize ; i++){
    printf("%c", '.');
  }
  printf("] %2.2f%%", progress);
	fflush(stdout);
}


/**
 * Sends a command that expects a 200 code response.
 * Appends \r\n to end of command.
 */

int send_command(int sockfd, char *cmd)
{
	int bytes;
	char buf[2048];
	strcpy(buf, cmd);
	strcat(buf, "\r\n");

	bytes = write(sockfd, buf, strlen(buf));
		if(bytes<0)	return -1; //not able to write
	bytes = read(sockfd, buf, 2048);
	int retcode;
	retcode = get_reply_code(buf);
	if (retcode != 200)
	{
		return retcode;
	}
	else
		return 0;
}

char get_file(URL_parsed URL)
{

	int psockfd, sockfd;
	struct sockaddr_in server_addr;
	int bytes;
	char *buf;
	struct addrinfo hints, *infoptr;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET; // AF_INET means IPv4 only addresses

	int result = getaddrinfo(URL.host, NULL, &hints, &infoptr);
	if (result)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		exit(1);
	}

	char ip[64];
	getnameinfo(infoptr->ai_addr, infoptr->ai_addrlen, ip, sizeof(ip), NULL, 0, NI_NUMERICHOST);

	freeaddrinfo(infoptr);

	printf("IP: %s\n", ip);
	printf("Press ENTER to continue...\n");
	getc(stdin);
	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(21); /*server TCP port must be network byte ordered */

	/*open a TCP socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if (connect(sockfd,
				(struct sockaddr *)&server_addr,
				sizeof(server_addr)) < 0)
	{
		perror("connect()");
		exit(0);
	}
	/*get a string from the server*/

	buf = malloc(2048 * sizeof(char));
	if (get_response(sockfd, NULL) == 120)
	{
		//if the server is busy, we wait
		if (get_response(sockfd, NULL) == 220)
		{
			//continue
		}
		else
		{
			//error
			exit(1);
		}
	}
	//send username
	sprintf(buf, "USER %s\r\n", URL.username);
	bytes = write(sockfd, buf, strlen(buf));


	if (get_response(sockfd, NULL) != 331)
	{
		printf("'USER username' command got reply %sExiting...\n", buf);
		exit(3);
	}

	// send password
	sprintf(buf, "PASS %s\r\n", URL.password);
	bytes = write(sockfd, buf, strlen(buf));
	if (get_response(sockfd, NULL) != 230)
	{
		printf("'PASS password' command got reply %sExiting...\n", buf);
		exit(3);
	}

	//passive mode to navigate the server

	sprintf(buf, "PASV\r\n");
	bytes=write(sockfd, buf , strlen(buf));
	char* retStr;

	buf[bytes]=0;
	if (get_response(sockfd, &retStr) != 227)
	{
		printf("'PASV' command got reply %sExiting...\n", buf);
		exit(3);
	}
	printf("getReply wrote to retStr: %s\n", retStr);

	unsigned short passPort = get_PASV_port(retStr);
	free(retStr);

	printf("Passive mode port is %d.\n", passPort);

	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(passPort); /*server TCP port must be network byte ordered */

	/*open a TCP socket*/
	if ((psockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if (connect(psockfd,
				(struct sockaddr *)&server_addr,
				sizeof(server_addr)) < 0)
	{
		perror("connect()");
		exit(0);
	}

	// if needed, we change directory
	if(strlen(URL.path) !=0){
		char cmd[1512];
		sprintf(cmd, "CWD %s", URL.path);
		send_command(sockfd, cmd);
	}else{
		send_command(sockfd, "CWD /");
	}
	send_command(sockfd, "TYPE I");

	bzero(buf, 2048);
	sprintf(buf, "RETR %s\r\n", URL.filename);
	bytes=write(sockfd, buf, strlen(buf));
	get_response(sockfd, &retStr);
	size_t filesize;
	char precise;
	precise = get_filesize_150(retStr, &filesize);
	free(retStr);
	size_t counter=0; //bytes read counter
	FILE* f;
	start();
	f=fopen(URL.filename, "w"); //gets the file
	while(1){
		bytes=read(psockfd, buf, 2048);
		if(bytes<=0)
			break;
		counter+=bytes;
		fwrite(buf, bytes, 1, f);
		if(precise !=-1) // error
			display(min(counter, filesize), filesize);
	}
	finish();
	if(precise){
		//last check for file size
		if((unsigned long)ftell(f)!=filesize){
			printf("File size does not match!\n");
			exit(-6);
		}
	}

	printf("File downloaded successfully!\n");
	fclose(f);
	close(psockfd);
	get_response(sockfd, NULL);
	close(sockfd);
	free(buf);
	exit(0);
}
