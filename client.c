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
#include <sys/time.h>

//define the host ip (same machine as the client code)
#define LOCALHOST "127.0.0.1"
//define the ephemeral port for client
#define EPHEMERAL_PORT "6666"

//this function is used to measure elapsed time in uS
long subtime(struct timeval t1, struct timeval t2) {
	return (t2.tv_sec - t1.tv_sec)*1000000 + t2.tv_usec- t1.tv_usec;
}

int main(int argc, char *argv[]) {
	//get the port number from program arguments
	int port = 0;
	if(argc<2) {
		printf("./client.c <port #>\n");
		return -1;
	}
	else {
		port = atoi(argv[1]);
		if((port <1) || (port >65535)) {
			printf("Invalid port number %d\n", port);
			return -1;
		}
		printf("Proceeding with port number %d\n", port);
	}
	
	//initialize relevant data structures
	int sockfd;
	struct addrinfo hints;
	struct addrinfo * serv;
	struct addrinfo *p;
	struct sockaddr_in addr;
	
	//dest: used to for getaddrinfo to populate struct
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, LOCALHOST, &addr.sin_addr);

	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;        
	
	int ret = getaddrinfo(LOCALHOST, argv[1], &hints, &serv);
	if(ret!=0) {
		printf("Error getaddrinfo with dest %d\n", ret);
		return -1;
	}
	//iterate through the linked list but accept the first valid entry and create a socket
	for(p = serv; p!=NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        	    perror("client: socket");
        	    continue;
        	}
        	break;
	}
	//exit if no entry found
	if(p==NULL) {
		printf("Error with socket()\n");
		return -1;
	}
	
	//repeat the same process for the source
	struct addrinfo * source;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	ret = getaddrinfo(NULL, EPHEMERAL_PORT, &hints, &source);
	if(ret!=0) {
		printf("Error getaddrinfo with src %d\n", ret);
		return -1;
	}
	
	//bind the socket to a file descriptor
	ret = bind(sockfd, source->ai_addr, source->ai_addrlen);
	if(ret != 0) {
    		printf("bind error %d\n", ret);
    		perror("client: socket");
		return -1;
	}
	
	struct timeval tv1, tv2;
	time_t curtime;

	//this part is just an example of how to send a message and not actually relevant
	//its here to clarify whats needed to send and ack the message and time it
	char * msg = "The Grasshopper Lies Heavy\n";
	gettimeofday(&tv1, NULL);	
	ret = sendto(sockfd, msg, strlen(msg) +1, 0, (struct sockaddr *)&addr, sizeof(addr));
	char buff[500];
	ret = recvfrom(sockfd, buff, 500, 0, (struct sockaddr *)&addr, NULL);
	gettimeofday(&tv2, NULL);
	long timer = subtime(tv1, tv2);
	printf("%s\n %ld\n", buff, timer);

	//using files to save the data from multiple runs and calculate the average times
	FILE *pFile;
	FILE *avFile;
	pFile = fopen("history.txt", "a+");
	avFile = fopen("average.txt", "a+");
	long avarr[5];  //change later
	char * line = malloc(100);	
	size_t size;
	for(int i=0; i<5; i++) {
		getline(&line, &size, avFile);
		avarr[i] = atol(line);
	}
	fclose(avFile);
	fseek ( pFile , 0 , SEEK_END);
	fprintf(pFile, "START\n");
	
	//for loop to test multiple different message sizes
	//if bounds of the loop change the txt files should be deleted because they will no longer make sense
	int j=0;
	for(int i=50; i<500; i+=100) {
		char rbuff[3000];
		char buf[i];
		memset(buf, 1, i);
		gettimeofday(&tv1, NULL);	
		ret = sendto(sockfd, buf, i +1, 0, (struct sockaddr *)&addr, sizeof(addr));
		ret = recvfrom(sockfd, rbuff, 3000, 0, (struct sockaddr *)&addr, NULL);
		gettimeofday(&tv2, NULL);
		long timer = subtime(tv1, tv2);
		printf("%d : elapsed time %ld\n", i, timer);
		fprintf(pFile, "%ld\n", timer);
		if(avarr[j] == 0)
			avarr[j] = timer;
		else
			avarr[j++] = (avarr[j] + timer)/2;
	}
	
	//save the calculated averages 
	fclose(pFile);
	avFile = fopen("average.txt", "w+");
	for(int i=0; i<5; i++) {
		fprintf(avFile, "%ld\n", avarr[i]);
	}
	fclose(avFile);

	//calculate R and Tprop here
	return 0;
}
