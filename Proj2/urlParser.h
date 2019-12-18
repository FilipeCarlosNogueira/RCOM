#pragma once

typedef struct
{
	char success;
	char protocol[6];
	char username[256];
	char password[256];
	char host[256];
	char path[1024];
	char filename[512];
} URL_parsed;

URL_parsed load_URL(char *url);
