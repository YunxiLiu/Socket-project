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

#define MYUDPPORT "21337"  // the udp port for the connection with backend servers
#define EDGEUDPPORT "24337"  // the udp port of back-end "AND" server
#define BACKLOG 10     // how many pending connections queue will hold
#define LOOPADDR "127.0.0.1" // my loopback address
#define MAXDATASIZE 20000 // max number of bytes we can get at once 
#define MAXBUFSIZE 200 // max number of bytes in the buffer 
#define MAXLINESIZE 200 // max number of bytes in one line
#define MAXLINENUM 200 // max number of lines
#define SENDFLAG 'y' // the flag to indicate that the results can be sent back at this time
#define LINENUMINDEX 3 // line prefix to mark line no.
#define OPERATORDIST 3 // size of "or "

// calculation for one line
void calculate(char *line, char **&results, int &curr_row_index) {
    results[curr_row_index] = new char[MAXLINESIZE];
    char * first = new char[MAXLINESIZE];
    char * second = new char[MAXLINESIZE];
    char * result = new char[MAXLINESIZE];
    int index = LINENUMINDEX + OPERATORDIST;
    int results_index = 0;
    int first_index = 0;
    int second_index = 0;

    // assign line number info to the results string
    while (results_index < LINENUMINDEX) {
        results[curr_row_index][results_index] = line[results_index];
        results_index++;
    }

    // get the first number
    while (line[index] != ',') {
        first[first_index] = line[index++];
        results[curr_row_index][results_index++] = first[first_index++];
    }
    first[first_index] = '\0';
    first_index--;
    index++;
    results[curr_row_index][results_index++] = ' ';
    results[curr_row_index][results_index++] = 'o';
    results[curr_row_index][results_index++] = 'r';
    results[curr_row_index][results_index++] = ' ';

    // get the second number
    while (index < strlen(line)) {
        second[second_index] = line[index++];
        results[curr_row_index][results_index++] = second[second_index++];
    }
    second[second_index] = '\0';
    second_index--;
    results[curr_row_index][results_index++] = ' ';
    results[curr_row_index][results_index++] = '=';
    results[curr_row_index][results_index++] = ' ';

    // do calculation and store the result to variable 'result'
    int max_len = first_index > second_index ? first_index: second_index;
    result[max_len + 1] = '\0';
    while (first_index >= 0 && second_index >= 0) {
        if (first[first_index] == '1' || second[second_index] == '1') {
            result[max_len--] = '1';
        } else {
            result[max_len--] = '0';
        }
        first_index--;
        second_index--;
    }
    while (first_index >= 0) {
        result[max_len--] = first[first_index--];
    }
    while (second_index >= 0) {
        result[max_len--] = second[second_index--];
    }

    // eliminate starting zeros
    int start_index = 0;
    while (start_index < strlen(result)) {
        if (result[start_index] == '1') {
            break;
        }
        start_index++;
    }
    // the calculation result is 0
    if (start_index == strlen(result)) {
        start_index--;
    }

    // assign the calculate result to the results string
    while (start_index < strlen(result)) {
        results[curr_row_index][results_index++] = result[start_index++];
    }
    results[curr_row_index][results_index] = '\0';

    printf("%.200s\n", &results[curr_row_index][LINENUMINDEX]);
    delete[] first;
    delete[] second;
    delete[] result;
    curr_row_index++;
}

int main(int argc, char *argv[])
{   
    // the following piece is referred from beej (line 114 to 151)
	struct addrinfo hints, *res, *res_edge, *p;
	int status;
	int udpfd; // socket for listening and socket for transmitting data
	struct sockaddr_storage client_addr; // connector's address information
	socklen_t clientaddr_size; // connector's address size
    char **results = new char*[MAXLINENUM]; // calculation results
    int curr_row_index = 0; // the current row of results

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; 

	if ((status = getaddrinfo(LOOPADDR, MYUDPPORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}
    if ((status = getaddrinfo(LOOPADDR, EDGEUDPPORT, &hints, &res_edge)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

	for(p = res; p != NULL; p = p->ai_next) {
        if ((udpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(udpfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(udpfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(res); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    
    printf("The Server OR is up and running using UDP on port %s.\n", MYUDPPORT);
    printf("The Server OR start receiving lines from the edge server for OR computation.\nThe computation results are:\n");
    
    while (1) {
        int numbytes = 0; // bytes number received
        char *buf = new char[MAXBUFSIZE]; // buffer for data
        if ((numbytes = recvfrom(udpfd, buf, MAXBUFSIZE-1 , 0, (struct sockaddr *)&client_addr, &clientaddr_size)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        buf[numbytes] = '\0';

        // deal with received data
        if (buf[0] != SENDFLAG) {
            calculate(buf, results, curr_row_index);
        } else {
            printf("The Server OR has successfully received %d lines from the edge server and finished all OR computations.\n", curr_row_index);
            for (int i = 0; i < curr_row_index; i++) {
                if (sendto(udpfd, results[i], strlen(results[i]), 0, res_edge->ai_addr, res_edge->ai_addrlen) == -1) {
                    perror("edge server: sendto");
                    exit(1);
                }
            }
            printf("The Server OR has successfully finished sending all computation results to the edge server\n");
            for (int i = 0; i < curr_row_index; i++) {
                delete [] results[i];
            }
            curr_row_index = 0;
        }
        delete[] buf;
    }
    close(udpfd);
    return 0;
}
