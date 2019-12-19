#include <string.h>
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
#include <ctype.h>

#include "getip.h"
#include "clientTCP.h"
#include "utils.h"

int main(int argc, char **argv){

    int max_length = 60;
    
    //user
	char user[max_length];
	char code[3];
	memset(user, 0, max_length);
    
    //password
	char pass[max_length];
	memset(pass, 0, max_length);
    
    //host
	char host[max_length];
	memset(host, 0, max_length);
    
    //path
	char path[max_length];
	memset(path, 0, max_length);

    //parse link values
	parseLink(argv[1], user, pass, host, path);
    
    //file name
	char filename[max_length];
    memset(filename, 0, max_length);
    
    //get file name from path
    getFilename(path, filename);

	printf("-> Username: %s\n", user);
	printf("-> Password: %s\n", pass);
	printf("-> Host: %s\n", host);
	printf("-> Path: %s\n", path);
	printf("-> Filename: %s\n", filename);

    struct hostent *h;
	h = getip(host);

    int socketfd;
    int socketfdClient =-1;
    int serverPort = 21;

    linkTCP(&socketfd, h, serverPort);

	getResponse(socketfd, code);
	if (code[0] == '2' && code[1]=='2' && code[2]=='0')
        printf("-> Connection Estabilished\n");

	printf("-> Sending Username\n");
	if(responseCodeManager(socketfd, "user ", user, filename, socketfdClient)){
		printf("-> Sending Password\n");
		responseCodeManager(socketfd, "pass ", pass, filename, socketfdClient);
	}

	write(socketfd, "pasv\n", 5);
	serverPort = getServerPort(socketfd);

    linkTCP(&socketfdClient, h, serverPort);   

	printf("\n-> Sending Retr\n");
	if(responseCodeManager(socketfd, "retr ", path, filename, socketfdClient)){
		close(socketfdClient);
		close(socketfd);
		exit(0);
	}
	else printf("-> ERROR!R ETR response failed!\n");

	close(socketfdClient);
	close(socketfd);
	exit(1);
}
