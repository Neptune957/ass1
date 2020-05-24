#pragma pack(4)

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/* You will to add includes here */


// Included to get the support library
#include <calcLib.h>

#include "protocol.h"
#define PORT "4950"  // the port users will be connecting to

#define BACKLOG 1  // how many pending connections queue will hold
#define SECRETSTRING "gimboid"

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]){
  int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage clientAddr; // connector's address information
  socklen_t sin_size;
  int yes=1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    printf("server: setting socket options\n");
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
      printf("server: set socket error\n");
      perror("setsockopt");
      exit(1);
    }

    printf("server: binding socket\n");
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      printf("server: bind socket error\n");
      continue;
    }

    break;
  }
  freeaddrinfo(servinfo);

  if (p == NULL)  {
    fprintf(stderr, "server: failed to bind\n");
    printf("server: fail to bind the socket\n");
    exit(1);
  }

  printf("server: listening to socekt\n");
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    printf("server:fail to listen to the socket\n");
    exit(1);
  }

  printf("server: waiting for connection...\n");
  char msg[1500];
  int MAXSZ=sizeof(msg)-1;
  
  int readSize;

  while(1) {  // main accept() loop
    sin_size = sizeof(clientAddr);
    new_fd = accept(sockfd, (struct sockaddr *)&clientAddr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      printf("server: accept connection error\n");
      continue;
    }

    printf("server: establish a connection from client\n");
    inet_ntop(clientAddr.ss_family,get_in_addr((struct sockaddr *)&clientAddr),s,sizeof s);

    printf("server: Sending support communication protocols \n");
    struct sockaddr_in *local_sin=(struct sockaddr_in*)&clientAddr;
    if (send(new_fd, "TEXT TCP 1.0\n", 13, 0) == -1){
      perror("send");
      printf("server: send error\n");
      shutdown(new_fd, SHUT_RDWR);
      close(new_fd);
      continue; //leave loop execution, go back to the while, main accept() loop. 
    }
    while(1){
      readSize=recv(new_fd,&msg,MAXSZ,0);
      printf("client: send status \"%s\"\n",msg);

      msg[readSize]='\0';
      if(strcmp(msg,"OK")!=0){
        printf("error:client does not support server\'s communication protocols\n");
        shutdown(new_fd, SHUT_RDWR);
        close(new_fd);
        break;
      }

      initCalcLib();
      char operationCommand[MAXSZ];
      char *operation=randomType();
      double fresult=0;
      int result=0;
      if(operation[0]=='f'){
        double value1=randomFloat(),value2=randomFloat();
        if(operation[1]=='a'){
          fresult=value1+value2;
        }else if(operation[1]=='s'){
          fresult=value1-value2;
        }else if(operation[1]=='m'){
          fresult=value1*value2;
        }else{
          fresult=value1/value2;
        }
        sprintf(operationCommand,"%s %f %f\0",operationCommand,value1,value2);
      }else{
        int value1=randomInt(),value2=randomInt();
        if(operation[0]=='a'){
          result=value1+value2;
        }else if(operation[0]=='s'){
          result=value1-value2;
        }else if(operation[0]=='m'){
          result=value1*value2;
        }else{
          result=value1/value2;
        }
        sprintf(operationCommand,"%s %d %d\0",operationCommand,value1,value2);
      }
      printf("server: send \"%s\" to client",operationCommand);

      if (send(new_fd, operationCommand, MAXSZ, 0) == -1){
        perror("send");
        printf("server: send error\n");
        shutdown(new_fd, SHUT_RDWR);
        close(new_fd);
        continue; //leave loop execution, go back to the while, main accept() loop. 
      }

      memset(msg,0,MAXSZ);
      readSize=recv(new_fd,&msg,MAXSZ,0);
      char *finalResponse;
      char resultString[MAXSZ];

      printf("client: my answer to \"operationCommand\" is: %s\n",msg);
      if(operation[0]=='f'){
        sprintf(resultString,"%8.8g\0",fresult);
      }else{
        sprintf(resultString,"%d\0",result);
      }

      if(strcmp(msg,resultString)==0){
        finalResponse="OK";
        printf("server: right answer\n");
      }else{
        finalResponse="ERROR";
        printf("server: wrong answer\n");
      }
      send(new_fd, finalResponse, sizeof(finalResponse), 0);
      shutdown(new_fd, SHUT_RDWR);
      close(new_fd);
      printf("server: mission has done. stop connection.\n");
      break;
    }
  }

  return 0;
}