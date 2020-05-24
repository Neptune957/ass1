#pragma pack(4)

#include <stdio.h>
#include <stdlib.h>
/* You will to add includes here */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Included to get the support library
#include <calcLib.h>
#include "protocol.h"

#define SERVERPORT "4950"	// the port users will be connecting to
#define MAXDATASIZE 1400

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]){
  
  	/* Do magic */
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;  
	char buf[MAXDATASIZE];
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		printf("client: server address reaping  failure\n");
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "clientmain: failed to create socket\n");
		printf("client: socket creation failure\n");
		exit(1);
	}

	if (connect(sockfd,p->ai_addr, p->ai_addrlen) < 0 ) {
	  	perror("clientmain: connect error.\n");
	  	printf("client: connetion failure\n");
	  	exit(1);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("client: connection established, connect to %s\n", s);


	freeaddrinfo(servinfo);

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	  	printf("Error in receiving data from server\n");
	  	close(sockfd);
	  	exit(1);
	}
	
	buf[numbytes] = '\0';
	
	printf("server: support communicating protocols as follows:\n '%s'\n",buf);
	char *serverSupportProtocol;
	bool supportServerProtocol=false;

	serverSupportProtocol = strtok(buf, "\n");
   
   	while( serverSupportProtocol != NULL ) {
     	if(strcmp(serverSupportProtocol,"TEXT TCP 1.0")==0){
     		char *response="OK";
     		send(sockfd, response, sizeof(response), 0);
     		supportServerProtocol=true;
     		printf("client: OK");
     		break;
     	}
      	serverSupportProtocol = strtok(NULL, "\n");
   	}
   	if(!supportServerProtocol){
   		printf("Client do not support server-side communicating protocol");
   		send(sockfd, "Not OK", 7, 0);
   		close(sockfd);
   		exit(1);
   	}

   	memset(buf,0,sizeof(buf));
   	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	  	printf("Error in receiving data from server");
	  	close(sockfd);
	  	exit(1);
	}
	char *operation;
	operation = strtok(buf, " ");
	double fresult;
	int result;
	char answer[100];
	if(operation[0]=='f'){
		double value1=atof(strtok(NULL, " "));
		double value2=atof(strtok(NULL, " "));
		if(operation[1]=='a'){
			fresult=value1+value2;
		}else if(operation[1]=='s'){
			fresult=value1-value2;
		}else if(operation[1]=='m'){
			fresult=value1*value2;
		}else{
			fresult=value1/value2;
		}
		sprintf(answer,"%8.8g\0",fresult);
		printf("client: receive command %s and value %f , %f from server\nSend answer %s back to server",operation,value1,value2,answer);
	}else{
		int value1=atoi(strtok(NULL, " "));
		int value2=atoi(strtok(NULL, " "));
		if(operation[0]=='a'){
			result=value1+value2;
		}else if(operation[0]=='s'){
			result=value1-value2;
		}else if(operation[0]=='m'){
			result=value1*value2;
		}else{
			result=value1/value2;
		}
		sprintf(answer,"%d\0",result);
		printf("client: receive command %s and value %d , %d from server\nSend answer %s back to server",operation,value1,value2,answer);
	}
	send(sockfd, answer, sizeof(answer), 0);

	memset(buf,0,sizeof(buf));
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	  	printf("Error in receiving data from server");
	  	close(sockfd);
	  	exit(1);
	}
	buf[numbytes] = '\0';
	printf("server: your answer is %s\n",buf);

	close(sockfd);
	printf("client: program shutdown.\n");


	return 0;

}
