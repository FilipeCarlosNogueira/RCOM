#include "getip.h"

//gets ip address according to the host's name
struct hostent *getip(char host[]){
	struct hostent *h;
    
    /*
	if (argc != 2) {
		fprintf(stderr,"usage: getip address\n");
		exit(1);
	}

	struct hostent {
	char    *h_name;	Official name of the host.
	  	char    **h_aliases;	A NULL-terminated array of alternate names for the host.
	int     h_addrtype;	The type of address being returned; usually AF_INET.
	  	int     h_length;	The length of the address in bytes.
	char    **h_addr_list;	A zero-terminated array of network addresses for the host.
				Host addresses are in Network Byte Order.
	};

	#define h_addr h_addr_list[0]	The first address in h_addr_list.
	*/

	if ((h = gethostbyname(host)) == NULL){
		herror("gethostbyname");
		exit(1);
	}

	// printf("Host name  : %s\n", h->h_name);
	printf("IP Address : %s\n",inet_ntoa(*((struct in_addr *)h->h_addr)));

	return h;
}
