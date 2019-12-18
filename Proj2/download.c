#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "urlParser.h"
#include "clientTCP.h"

int main(int argc, char **argv)
{
	if (argc != 2){
		write(STDOUT_FILENO, "Correct usage: Please use ./a.out <FTPurl>\n", 50);
		return -1;
	}

	URL_parsed URL = load_URL(argv[1]);

	if (URL.success == 0){
		write(STDOUT_FILENO, "The passed URL was malformed. Please use the following format:\n\tftp://[<user>:<password>@]<host>/<url-path>\n", 109);
		return 0;
	}

	printf("protocol: 	%s\n", URL.protocol);
	printf("user:		%s\n", URL.username);
	printf("password:	%s\n", URL.password);
	printf("host:		%s\n", URL.host);
	printf("path:		%s\n", URL.path);
	printf("filename:	%s\n", URL.filename);
	
	char gotFile = get_file(URL);

	if(gotFile!=0)
		return gotFile;

	return 0;
}
