#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>


#define MAXDATASIZE 1024 // max buffer size
#define SERVER_PORT 2000

int client_socket(char *hostname)
{
  char port[20];
  struct addrinfo their_addrinfo; // server address info
  struct addrinfo *their_addr = NULL; // connector's address information
  int sockfd;

  /////////////////////////

  //TODO:
  // 1) initialise socket using 'socket'
  // 2) initialise 'their_addrinfo' and use 'getaddrinfo' to lookup the IP from host name
  // 3) connect to remote host using 'connect'

  ///////////////////////////////

  /*
  struct sockaddr_in server_addr;


  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(hostname);
  server_addr.sin_port = htons(SERVER_PORT);


  connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  sprintf(port, "%d", SERVER_PORT);
  memset(&their_addrinfo, 0, sizeof(struct addrinfo));

  their_addrinfo.ai_family = AF_INET;
  their_addrinfo.ai_socktype = SOCK_STREAM;

  getaddrinfo(hostname, port, &their_addrinfo, &their_addr);
  connect(sockfd, their_addr->ai_addr, their_addr->ai_addrlen);



  return sockfd;
}


int main(int argc, char *argv[])
{
    char buffer[MAXDATASIZE];
    int pid;

    if (argc != 2) {
      fprintf(stderr,"usage: client hostname\n");
      exit(1);
    }

    int sockfd = client_socket(argv[1]);

    int numbytes = 0;
    char *line;


    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        exit(-1);
    }
    else if (pid == 0)
    {
        do {

          line = readline(">> ");
          write(sockfd, line, strlen(line));

      } while(1);
    }
    else
    {
        do {
            numbytes = read(sockfd, buffer, MAXDATASIZE - 1);
            buffer[numbytes] = '\0';

            printf("server: %s\n", buffer);
            printf(">> ");
            fflush(stdout);
        } while(numbytes > 0);

        free(line);
        close(sockfd);
    }




    return 0;
}
