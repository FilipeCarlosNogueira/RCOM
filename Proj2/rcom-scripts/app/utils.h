#pragma once

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

void newFile(int fd, char* filename);
void getResponse(int socketfd, char *responseCode);
int responseCodeManager(int socketfd, char str[], char value[], char* filename, int socketfdClient);
int getServerPort(int socketfd);
void getFilename(char *path, char *filename);
void parseLink(char *link, char *user, char *pass, char *host, char *path);

