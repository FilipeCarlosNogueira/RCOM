#include "utils.h"

void newFile(int fd, char* filename){
    int buf_size = 1000;

	FILE *new = fopen((char *)filename, "wb+");

	char socket[buf_size];
 	int bytes;
 	while ((bytes = read(fd, socket, buf_size))>0)
    	fwrite(socket, bytes, 1, new);

  	fclose(new);

	printf("-> Download complete!\n");
}

//gets response code from the server
void getResponse(int socketfd, char *code){
	int state = 0;
	int index = 0;
	char c;

	while (state != 3){
		read(socketfd, &c, 1);
		printf("%c", c);
		
        switch (state){
		    //reads 3 digit response code
		    case 0:
			    if (c == ' '){
				    if (index != 3){
					    printf(" ---Error! Incorrect response code!\n");
					    return;
				    }
				    index = 0;
				    state = 1;
			    }
			    else{
				    if (c == '-'){
					    state = 2;
					    index=0;
				    }
				    else if (isdigit(c)){
				        code[index] = c;
				        ++index;
				    }
			    }
		    break;

		    // reaches the end of line
		    case 1:
			    if (c == '\n') state = 3;
            break;

		    // reads message
		    case 2:
			    if (c == code[index]) ++index;
			    else{
			        if (index == 3 && c == ' ')
                        state = 1;
			        else if(index==3 && c=='-')
                        index=0;
		        }
	        break;
		}
	}
}

//sends a command, reads the response from the server and interprets it
int responseCodeManager(int socketfd, char str[], char value[], char* filename, int socketfdClient){
	char code[3];
	int action = 0;

	//sends the command
	write(socketfd, str, strlen(str));
	write(socketfd, value, strlen(value));
	write(socketfd, "\n", 1);

	while (1){

        //reads the response
		getResponse(socketfd, code);
		action = code[0] - '0';

		switch (action){
		    //waits for another response
		    case 1:
			    if(strcmp(str, "retr ")==0){
				    newFile(socketfdClient, filename);
				    break;
			    }
			    getResponse(socketfd, code);
		    break;

		    //command accepted
		    case 2:
			    return 1;
            break;

		    //needs additional information
		    case 3:
			    return -1;
            break;

		    //try again
		    case 4:
			    write(socketfd, str, strlen(str));
			    write(socketfd, value, strlen(value));
			    write(socketfd, "\r\n", 2);
		    break;

		    case 5:
			    printf("-> ERROR! Command invalid!\n");
			    close(socketfd);
			    exit(-1);
            break;
		}
	}
}

//reads the server port when pasv is sent
int getServerPort(int socketfd){
	
    char byteOne[4];
	memset(byteOne, 0, 4);
	
    char byteTwo[4];
	memset(byteTwo, 0, 4);
    
    int state = 0;
	int index = 0;
	char c;

	while (state != 7){

		read(socketfd, &c, 1);
		printf("%c", c);

		switch (state){

		    //waits for 3 digit number followed by ' '
		    case 0:
			    if (c == ' '){
				    if (index != 3){
					    printf("-> Error! Receiving response code failed!\n");
					    return -1;
				    }
				    index = 0;
				    state = 1;
			    }
			    else index++;
		    break;

		    case 5:
			    if (c == ','){
				    index = 0;
				    state++;
			    }
			    else{
				    byteOne[index] = c;
				    index++;
			    }
		    break;

		    case 6:
			    if (c == ')') state++;
			    else{
				    byteTwo[index] = c;
				    index++;
			    }
		    break;

		    //reads until the first comma
		    default:
			    if (c == ',') state++;
		    break;
		}
	}

	return (atoi(byteOne) * 256 + atoi(byteTwo));
}

void getFilename(char *path, char *filename){
    
    int pos = 0;
    for(int i = 0; i < strlen(path); ++i)
        if(path[i] == '/') pos = i;

    memcpy(filename, &path[pos+1], strlen(path)-pos);
}

void parseLink(char *link, char *user, char *pass, char *host, char *path){
	
    char init[] = "ftp://";
	int pos = 0;
	int state = 0;

	for(int i = 0; i < strlen(link); ++i){
		switch (state){
		    case 0: //checks the ftp://
			    if (link[i] == init[i] && i < 5)
			        break;
			    if (i == 5 && link[i] == init[i])
				    state = 1;
			    else
				    printf(" -- Error! Parsing ftp:// failed!");
			break;

		    case 1: //username
			    if (link[i] == ':'){
				    state = 2;
				    pos = 0;
			    }
			    else{
				    user[pos] = link[i];
				    pos++;
			    }
			break;

		    case 2: //password
			    if (link[i] == '@'){
				    state = 3;
				    pos = 0;
			    }
			    else{
				    pass[pos] = link[i];
				    pos++;
			    }
			break;

		    case 3: //host
			    if (link[i] == '/'){
				    state = 4;
				    pos = 0;
			    }
			    else{
				    host[pos] = link[i];
				    pos++;
			    }
			break;

		    case 4: //path
			    path[pos] = link[i];
			    pos++;
			break;
		}
	}
}
