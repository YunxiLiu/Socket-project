#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/wait.h>

#define SERVERPORT "23337" // tcp port number at the edge server
#define LOOPADDR "127.0.0.1" // my loopback address
#define MAXDATASIZE 20000 // max number of bytes we can get at once 
#define MAXBUFSIZE 200 // max number of bytes we can get at once 

// this function is copied from beej's ref
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{	
	// the following pieces of code is referred from beej (line 32 to 73)
	struct addrinfo hints, *res, *p;
	int status;
	int yes = 1;
	int sockfd; // file descriptor of socket

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	// use getaddrinfo() to get address structs
	if ((status = getaddrinfo(LOOPADDR, SERVERPORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}

	// go through the address results and connect
	for (p = res; p != NULL; p = p->ai_next) {
		// create socket
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      		perror("setsockopt");
      		exit(1);
    	}
		// connect to the server using the established socket
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		
		break;
	}

	// not any address is available
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	freeaddrinfo(res); // all done with this structure
	printf("The client is up and running.\n");

	// file transfer
	FILE * pFile;
	pFile = fopen (argv[1], "r");
	if (pFile == NULL) {
		perror ("Error opening file");
		return 3;
	}

	int numLines = 0;
	
	while (!feof(pFile)) {
		char *buffer = new char[MAXBUFSIZE]; // store the data in file
		fgets(buffer, MAXBUFSIZE, pFile);

		int len = strlen(buffer);
		if (len == 1) {
			break;		
		}
		if (send(sockfd, buffer, len, 0) == -1) {
			perror("send");
			exit(EXIT_FAILURE);
		}
		
		if (len > 0 && (buffer[len - 1] == '\n')) {
			numLines++;
		}
	}
	fclose (pFile); // close the txt file
	printf("The client has successfully finished sending %d lines to the edge server.\n", numLines);
	
	
	// receive results
	char *buffer_recv = new char[MAXBUFSIZE];
    char *results = new char[MAXDATASIZE];
    int offset = 0;
    
	while (1) { // while the stream goes on
        int num_bytes = 0; // number of bytes received each time
        num_bytes = recv(sockfd, buffer_recv, MAXBUFSIZE-1, 0);
        //printf("received something\n");
        if (num_bytes == -1) {
      		perror("recv");
       		exit(1);
   		}
    	if (num_bytes == 0) { // no data in buffer
   			break;
   		}

    	//store data in the buffer to result
 		for (int i = 0; i < num_bytes; ++i) {
    		results[offset + i] = buffer_recv[i];
    	}
    	offset += num_bytes;
    }
    results[offset] = '\0';
    printf("The client has successfully finished receiving all computation results from the edge server.\n");
	printf("The final computation results are:\n%s",results);
	close(sockfd);
	return 0;
}
