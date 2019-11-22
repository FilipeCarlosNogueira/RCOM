#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#define SERVER_PORT 21
#define SERVER_ADDR "192.168.28.96"

void read_response(int sockfd, char *response);
struct hostent *get_ip(char host[]);
void generate_file(int fd, char* filename);
void argument_parser(char *argument, char *user, char *pass, char *host, char *path);
int send_command(int socketfd, char cmd[], char commandContent[], char* filename, int socketfdClient);
int get_server_port(int socketfd);
void filename_parser(char *path, char *filename);

int main(int argc, char **argv)
{
	int sockfd;
	int sockfd_client = -1;
	struct sockaddr_in server_addr;
	struct sockaddr_in server_addr_client;
	struct hostent *h;

	char user[50]; memset(user, 0, 50);
	char pass[50]; memset(pass, 0, 50);
	char host[50]; memset(host, 0, 50);
	char path[50]; memset(path, 0, 50);

	argument_parser(argv[1], user, pass, host, path);

	char filename[50]; filename_parser(path, filename);
    char response[3];

	printf("Username: %s\n", user);
	printf("Password: %s\n", pass);
	printf("Host: %s\n", host);
	printf("Path :%s\n", path);
	printf("Filename: %s\n", filename);

	h = getip(host);

	printf("IP Address : %s\n\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

	/*server address handling*/
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr))); /*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);											/*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("connect()");
		exit(0);
	}

	read_response(sockfd, response); 

	if (response[0] == '2')
	{										 
		printf("Connection Estabilished!\n"); 
	}										 

	printf("Sending Username...");
	int res = sendCommandInterpretResponse(sockfd, "user ", user, filename, sockfd_client);
    printf(" OK!\n");
	if (res == 1)
	{
		printf("Sending Password...");
		res = sendCommandInterpretResponse(sockfd, "pass ", pass, filename, sockfd_client);
        printf(" OK!\n");
	}

	write(sockfd, "pasv\n", 5);

	int server_port = get_server_port(sockfd);

	/*server address handling*/
	bzero((char *)&server_addr_client, sizeof(server_addr_client));
	server_addr_client.sin_family = AF_INET;
	server_addr_client.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)h->h_addr))); /*32 bit Internet address network byte ordered*/
	server_addr_client.sin_port = htons(server_port);										   /*server TCP port must be network byte ordered */

	/*open an TCP socket*/
	if ((sockfd_client = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket()");
		exit(0);
	}
	/*connect to the server*/
	if (connect(sockfd_client, (struct sockaddr *)&server_addr_client, sizeof(server_addr_client)) < 0)
	{
		perror("connect()");
		exit(0);
	}
	printf("\nSending Retr...\n");
	int Retr_response = send_command(sockfd, "retr ", path, filename, sockfd_client);
    printf(" OK!\n");

	if(Retr_response == 0)
    {
		close(sockfd_client);
		close(sockfd);
		exit(0);
	}
	else 
        printf("ERROR in Retr response.\n");

	close(sockfd_client);
	close(sockfd);
	exit(1);
}

// ./download ftp://anonymous:1@speedtest.tele2.net/1KB.zip
void argument_parser(char *argument, char *user, char *pass, char *host, char *path)
{
	char ftp[] = "ftp://";
	int id = 0, i = 0, state = 0, length = strlen(argument);

	while (i < length)
	{
		switch (state)
		{
		case 0: //reads "ftp://"
			if (argument[i] == ftp[i] && i < 5)
			{
				break;
			}
			if (i == 5 && argument[i] == ftp[i])
				state = 1;
			else
				printf("Error parsing 'ftp://' !\n");
			break;
		case 1: //reads username
			if (argument[i] == ':')
			{
				state = 2;
				id= 0;
			}
			else
			{
				user[id] = argument[i];
				id++;
			}
			break;
		case 2:
			if (argument[i] == '@')
			{
				state = 3;
				id = 0;
			}
			else
			{
				pass[id] = argument[i];
				id++;
			}
			break;
		case 3:
			if (argument[i] == '/')
			{
				state = 4;
				id = 0;
			}
			else
			{
				host[id] = argument[i];
				id++;
			}
			break;
		case 4:
			path[id] = argument[i];
			id++;
			break;
		}
		i++;
	}
}

void filename_parser(char *path, char *filename){
	int path_id = 0, filename_id = 0;

	memset(filename, 0, 50);

	while(path_id< strlen(path))
    {
		if(path[path_id]=='/')
        {
			filename_id = 0;
			memset(filename, 0, 50);
		}
		else
        {
			filename[filename_id] = path[path_id];
			filename_id++;
		}

        path_id++;
	}
}

//gets ip address depending on the host's name
struct hostent *get_ip(char host[])
{
	struct hostent *h;

	if ((h = gethostbyname(host)) == NULL)
	{
		herror("gethostbyname");
		exit(1);
	}

	return h;
}

//reads response from the server
void read_response(int sockfd, char *response)
{
	int state = 0, id = 0;
	char c;

	while (state != 3)
	{	
		read(sockfd, &c, 1);
		printf("%c", c);
        
		switch (state)
		{
		//waits for 3 digit number followed by ' ' or '-'
		case 0:
			if (c == ' ')
			{
				if (id != 3)
				{
					printf("Error receiving response code!\n");
					return;
				}
				id = 0;
				state = 1;
			}
			else
			{
				if (c == '-')
				{
					state = 2;
					id=0;
				}
				else
				{
					if (isdigit(c))
					{
						response[id] = c;
						id++;
					}
				}
			}
			break;
		//reads until end of the line
		case 1:
			if (c == '\n')
			{
				state = 3;
			}
			break;
		//waits for response code in multiple line responses
		case 2:
			if (c == response[id])
			{
				id++;
			}
			else
			{
				if (id == 3 && c == ' ')
				{
					state = 1;
				}
				else 
				{
				  if(id==3 && c=='-')
                  {
					id=0;
				  }
				}
				
			}
			break;
		}
	}
}

//when pasv is sent, reads the server port
int get_server_port(int sockfd)
{
	int state = 0, id = 0;

	char first_byte[4]; memset(first_byte, 0, 4);
	char second_byte[4]; memset(second_byte, 0, 4);
	char c;

	while (state != 7)
	{
		read(sockfd, &c, 1);
		printf("%c", c);
		switch (state)
		{
		//waits for 3 digit number followed by ' '
		case 0:
			if (c == ' ')
			{
				if (id != 3)
				{
					printf("Error receiving response code!\n");
					return -1;
				}
				id = 0;
				state = 1;
			}
			else
			{
				id++;
			}
			break;
		case 5:
			if (c == ',')
			{
				id = 0;
				state++;
			}
			else
			{
				first_byte[id] = c;
				id++;
			}
			break;
		case 6:
			if (c == ')')
			{
				state++;
			}
			else
			{
				second_byte[id] = c;
				id++;
			}
			break;
		//reads until the first ,
		default:
			if (c == ',')
			{
				state++;
			}
			break;
		}
	}

	int int_first_byte = atoi(first_byte), int_second_byte = atoi(second_byte);

	return (256 * int_first_byte + int_second_byte);
}

//sends a command, reads the response from the server and interprets it
int send_command(int sockfd, char command[], char command_data[], char* filename, int sockfd_client)
{
	char response[3];
	int n = 0;

	//sends the command
	write(sockfd, command, strlen(command));
	write(sockfd, command_data, strlen(command_data));
	write(sockfd, "\n", 1);

	while (1)
	{
		//reads the response
		read_response(sockfd, response);
		n = response[0] - '0';

		switch (n)
		{
		//waits for another response
		case 1:
			if(strcmp(command, "retr ")==0){
				generate_file(sockfd_client, filename);
				break;
			}
			read_response(sockfd, response);
			break;
		//command accepted, we can send another command
		case 2:
			return 0;
		//needs additional information
		case 3:
			return 1;
		//try again
		case 4:
			write(sockfd, command, strlen(command));
			write(sockfd, command_data, strlen(command_data));
			write(sockfd, "\r\n", 2);
			break;
		case 5:
			printf("Command wasn't accepted! Exiting...\n");
			close(sockfd);
			exit(-1);
		}
	}
}

void generate_file(int fd, char* filename)
{
	FILE *f = fopen((char *)filename, "wb+");

	char socket_buffer[1000];
 	int bytes;

 	while ((bytes = read(fd, socket_buffer, 1000))>0) 
	{
    	bytes = fwrite(socket_buffer, bytes, 1, f);
    }

  	fclose(f);

	printf("File Download Complete!\n");
}
