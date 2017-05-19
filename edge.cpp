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

#define TCPPORT "23337"  // the tcp port for the connection with client
#define MYUDPPORT "24337"  // the udp port for the connection with backend servers
#define ANDUDPPORT "22337"  // the udp port of back-end "AND" server
#define ORUDPPORT "21337"  // the udp port of back-end "OR" server
#define BACKLOG 10     // how many pending connections queue will hold
#define MAXDATASIZE 20000 // max number of bytes we can get at once 
#define MAXBUFSIZE 200 // max number of bytes in the buffer 
#define MAXLINESIZE 200 // max number of bytes in the one line of data 
#define MAXLINENUM 200 // max number of lines in the original file 
#define LOOPADDR "127.0.0.1" // my loopback address
#define SENDFLAG "y" // the flag to indicate that the results can be sent back at this time
#define LINENUMINDEX 3 // line prefix to mark line no.

// convert from line prefix chars to line number
int convert_to_int(char *oneLine) {
	int num = 0;
	num = (oneLine[0] - '0') * 100 + (oneLine[1] - '0') * 10 + (oneLine[2] - '0');
	return num;
}

// convert from line number to line prefix chars
void num_to_threeDigit(char *& line, int &index, int line_num) {
	int digit = 100;
	while (digit > 0) {
		line[index++] = '0' + line_num / digit;
		line_num %= digit;
		digit /= 10;
	}
}

//convert from file text to lines
void file_to_lines(char *file_text, char **&and_lines, char **&or_lines, int &and_line_num, int &or_line_num) {
	and_lines = new char*[MAXLINENUM];
	or_lines = new char*[MAXLINENUM];
	int size = strlen(file_text);
	int file_index = 0;
	int line_num = 0;
	while (file_index < size) {
		if (file_text[file_index] == 'a') {
			and_lines[and_line_num] = new char[MAXLINESIZE];
			int line_index = 0;
			num_to_threeDigit(and_lines[and_line_num], line_index, line_num);
			while (file_index < size && file_text[file_index] != '\n') {
				and_lines[and_line_num][line_index++] = file_text[file_index++];
			}
			and_lines[and_line_num][line_index] = '\0';
			and_line_num++;
			line_num++;
		} else if (file_text[file_index] == 'o') {
			or_lines[or_line_num] = new char[MAXLINESIZE];
			int line_index = 0;
			num_to_threeDigit(or_lines[or_line_num], line_index, line_num);
			while (file_index < size && file_text[file_index] != '\n') {
				or_lines[or_line_num][line_index++] = file_text[file_index++];
			}
			or_lines[or_line_num][line_index] = '\0';
			or_line_num++;
			line_num++;
		} else {
			file_index++;
		}
	}
}

// this function is copied from beej
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
	struct addrinfo hints, *res, *res_and, *res_or, *p;
	int status;
	int tcpfd, childfd, udpfd; // socket for listening and socket for transmitting data
	struct sockaddr_storage client_addr; // connector's address information
	socklen_t clientaddr_size;
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 

	// set up tcp socket
	// get my tcp port info
	if ((status = getaddrinfo(LOOPADDR, TCPPORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}

	for(p = res; p != NULL; p = p->ai_next) {
        if ((tcpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (bind(tcpfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(tcpfd);
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
    if (listen(tcpfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    printf("The edge server is up and running.\n");

    //set up udp socket
    //my udp port info
    hints.ai_socktype = SOCK_DGRAM; 
  	hints.ai_flags = 0;
  	if ((status = getaddrinfo(LOOPADDR, MYUDPPORT, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo(my udp): %s\n", gai_strerror(status));
		return 1;
	}
	for (p = res; p != NULL; p = p->ai_next) {
		//create socket
		if ((udpfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		// bind my udp socket using info of my port number
		if (bind(udpfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(udpfd);
           	perror("client: bind");
           	continue;
       	}
		break;
	}
	if (p == NULL) {
   		fprintf(stderr, "edge server: failed to bind udp socket\n");
    	return 2;
  	}
	freeaddrinfo(res); // all done with this structure

	// get the back end "AND" server info
	if ((status = getaddrinfo(LOOPADDR, ANDUDPPORT, &hints, &res_and)) != 0) {
		fprintf(stderr, "getaddrinfo(my udp): %s\n", gai_strerror(status));
		return 1;
	}
	if (res_and == NULL) {
    	fprintf(stderr, "edge server: failed to get backend_AND server udp socket\n");
   		return 2;
  	}
  	// get the back end "OR" server info
  	if ((status = getaddrinfo(LOOPADDR, ORUDPPORT, &hints, &res_or)) != 0) {
		fprintf(stderr, "getaddrinfo(my udp): %s\n", gai_strerror(status));
		return 1;
	}
	if (res_or == NULL) {
    	fprintf(stderr, "edge server: failed to get backend_OR server udp socket\n");
   		return 2;
  	}

    // always listening and accepting
    while(1) {
        clientaddr_size = sizeof client_addr;
        childfd = accept(tcpfd, (struct sockaddr *)&client_addr, &clientaddr_size);
        if (childfd == -1) {
            perror("accept");
            continue;
        }
        if (setsockopt(childfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        // each file transfer
        int numLines = 0; // number of lines received
        int offset = 0; // start index of the new char array where we store data 
        char *file_text = new char[MAXDATASIZE];
        char **results = new char*[MAXLINENUM]; // calculation results
        char **sorted_results = new char*[MAXLINENUM];
        while (1) { // while the stream goes on
        	char buf[MAXBUFSIZE];
        	memset(buf,0,sizeof(buf));
        	int numbytes = 0; // number of bytes received each time
        	numbytes = recv(childfd, buf, MAXBUFSIZE-1, 0);
            if (numbytes == -1) {
                if (offset >= 1 && file_text[offset - 1] != '\n') {
                    numLines++;
                }
        		break;
    		}
    		if (numbytes == 0) { // no data in buffer
    			if (offset >= 1 && file_text[offset - 1] != '\n') {
    				numLines++;
    			}
    			break;
    		}

    		//store data in the buffer to result
    		for (int i = 0; i < numbytes; ++i)
    		{
    			file_text[offset + i] = buf[i];
    			if (buf[i] == '\n') {
    				numLines++;
    			}
    		}
    		offset += numbytes;
    	}
    	file_text[offset] = '\0';
    	printf("The edge server has received %d lines from the client using TCP over port %s.\n", numLines, TCPPORT);

    	// send data to backend servers
    	char **and_lines;
    	char **or_lines;
    	int and_line_num = 0;
    	int or_line_num = 0;
    	file_to_lines(file_text, and_lines, or_lines, and_line_num, or_line_num); // convert the file text to lines
    	for (int i = 0; i < and_line_num; i++) {
    		if (sendto(udpfd, and_lines[i], strlen(and_lines[i]), 0, res_and->ai_addr, res_and->ai_addrlen) == -1) {
				perror("edge server: sendto");
    			exit(1);
  			}
    	}
    	printf("The edge has successfully sent %d lines to Backend-Server AND.\n", and_line_num);
    	for (int i = 0; i < or_line_num; i++) {
    		if (sendto(udpfd, or_lines[i], strlen(or_lines[i]), 0, res_or->ai_addr, res_or->ai_addrlen) == -1) {
				perror("edge server: sendto");
    			exit(1);
  			}
    	}
    	printf("The edge has successfully sent %d lines to Backend-Server OR.\n", or_line_num);

    	// clear the memory
    	for (int i = 0; i < and_line_num; i++) {
    		delete [] and_lines[i];
		}
		delete [] and_lines;

		for (int i = 0; i < or_line_num; i++) {
    		delete [] or_lines[i];
		}
		delete [] or_lines;
		delete [] file_text;

    	printf("The edge server start receiving the computation results from Backend-Server OR and Backend-Server AND using UDP port %s.\n",MYUDPPORT);
    	printf("The computation results are:\n");

    	char buf[MAXBUFSIZE];
  		int numbytes = 0;

  		// send request to fetch data from backend_OR servers
    	if (sendto(udpfd, SENDFLAG, strlen(SENDFLAG), 0, res_or->ai_addr, res_or->ai_addrlen) == -1) {
			perror("edge server: sendto");
    		exit(1);
  		}
  		// receive results
  		for (int i = 0; i < or_line_num; i++) {
			if ((numbytes = recvfrom(udpfd, buf, MAXBUFSIZE-1 , 0, (struct sockaddr *)&client_addr, &clientaddr_size)) == -1) {
           		perror("recvfrom");
           		exit(1);
  			}
  			buf[numbytes] = '\0';
  			results[i] = new char[MAXLINESIZE];
  			strcpy(results[i], buf);
  			printf("%.200s\n",&results[i][LINENUMINDEX]);
  		}

    	// send request to fetch data from backend_AND servers
    	if (sendto(udpfd, SENDFLAG, strlen(SENDFLAG), 0, res_and->ai_addr, res_and->ai_addrlen) == -1) {
			perror("edge server: sendto");
    		exit(1);
  		}
  		// receive results
  		for (int i = 0; i < and_line_num; i++) {
			if ((numbytes = recvfrom(udpfd, buf, MAXBUFSIZE-1 , 0, (struct sockaddr *)&client_addr, &clientaddr_size)) == -1) {
           		perror("recvfrom");
           		exit(1);
  			}
  			buf[numbytes] = '\0';
  			results[i + or_line_num] = new char[MAXLINESIZE];
  			strcpy(results[i + or_line_num], buf);
  			printf("%.200s\n",&results[i + or_line_num][LINENUMINDEX]);
  		}
  		printf("The edge server has successfully finished receiving all computation results from the Backend-Server OR and Backend-Server AND.\n");

  		// convert to sorted results
  		for (int i = 0; i < numLines; i++) {
  			int this_line = convert_to_int(results[i]);
  			sorted_results[this_line] = new char[MAXLINESIZE];
  			//printf("this_line:%d\n", this_line);
  			char *temp_result = new char[MAXLINESIZE];
  			int temp_result_index = 0;
  			int sorted_results_index = 0;
  			for (int j = strlen(results[i]) - 1; results[i][j] != ' '; j--) {
  				temp_result[temp_result_index++] = results[i][j];
  			}
  			temp_result[temp_result_index] = '\0';
  			temp_result_index--;
  			while(temp_result_index >= 0) {
  				sorted_results[this_line][sorted_results_index++] = temp_result[temp_result_index--];
  			}
  			sorted_results[this_line][sorted_results_index++] = '\n';
            sorted_results[this_line][sorted_results_index] = '\0';
  			//printf("sorted_results line %d:%s\n", this_line,sorted_results[this_line]);
  		}
  		
  		// send the results back to the client
		for (int i = 0; i < numLines; i++) {
			if (send(childfd, sorted_results[i], strlen(sorted_results[i]), 0) == -1) {
				perror("send");
				exit(EXIT_FAILURE);
			}
			//printf("send one line success!!\n");
		}
		printf("The edge server has successfully finished sending all computation results to the client.\n");
		
		for (int i = 0; i < numLines; i++) {
    		delete [] results[i];
    		delete [] sorted_results[i];
		}
        close(childfd);
    } // outer listening while loop
    return 0;
}
