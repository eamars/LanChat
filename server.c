#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1024 // max buffer size
#define SERVER_PORT 2000

int listen_on(int port)
{

	int s = socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;          /* communicate using internet address */
	sa.sin_addr.s_addr = INADDR_ANY; /* accept all calls                   */
	sa.sin_port = htons(port); /* this is port number                */

	int rc = bind(s,(struct sockaddr *)&sa,sizeof(sa)); /* bind address to socket   */
	if(rc == -1) { // Check for errors
		perror("bind");
		exit(1);
	}

	rc = listen (s, 5); /* listen for connections on a socket */
	if(rc == -1) { // Check for errors
		perror("listen");
		exit(1);
	}

	return s;
}


int accept_connection(int s) {

	/////////////////////////////////////////////
	// TODO: Implement in terms of 'accept'

	/////////////////////////////////////////////
	int client_fd;
	struct sockaddr_in client_sock;
	socklen_t sz = sizeof(struct sockaddr_in);

	client_fd = accept(s, (struct sockaddr *) &client_sock, &sz);
	return client_fd;
}


void handle_request(int msgsock, int child_to_parent) {
	///////////////////

	// TODO: write a function to reply to all incoming messages
	// while the connection remains open

	///////////////////
	char buffer[MAXDATASIZE];
	int sz;
	int ret;
	fd_set set;

	// get peer name
	/*
	char ipstr[INET6_ADDRSTRLEN];
	struct sockaddr_in *address;
	socklen_t address_len = sizeof(*address);
	getpeername(msgsock, (struct sockaddr *) address, &address_len);
	inet_ntop(AF_INET, &address->sin_addr, ipstr, sizeof(ipstr));
	printf("IP: [%s], PORT: [%d]\n", ipstr, ntohs(address->sin_port));
	*/

	while (1)
	{
		FD_ZERO(&set);
		FD_SET(msgsock, &set);

		if ((ret = select(FD_SETSIZE, &set, NULL, NULL, NULL)) < 0)
		{
			// select can catch the signal but we want to ignore it
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				perror("select");
				exit(-1);
			}

		}
		if (FD_ISSET(msgsock, &set))
		{
			memset(buffer, 0, MAXDATASIZE);
			sz = read(msgsock, buffer, MAXDATASIZE);
			if (sz <= 0)
			{
				break;
			}
			printf("RECV: [%s]\n", buffer);

			// send back
			write(msgsock, buffer, strlen(buffer));

			// send to parent
			write(child_to_parent, buffer, strlen(buffer));
		}



	}
	printf("Client disconnected\n");
	close(msgsock);
}

int main(int argc, char *argv[])
{
	signal(SIGCHLD, SIG_IGN);

	int ret;
	int i;
	int pid;
	int sock_fd;
	int child_to_parent[2]; // pipe for child to communicate with parent
	char buffer[MAXDATASIZE];

	if (pipe(child_to_parent) < 0)
	{
		perror("pipe");
		exit(-1);
	}

	printf("\nThis is the server with pid %d listening on port %d\n", getpid(), SERVER_PORT);

	// setup the server to bind and listen on a port
	sock_fd = listen_on(SERVER_PORT);

	fd_set set;


	while(1) { // forever
		FD_ZERO(&set);
		FD_SET(sock_fd, &set);
		FD_SET(child_to_parent[0], &set);
		if ((ret = select(FD_SETSIZE, &set, NULL, NULL, NULL)) < 0)
		{
			// select can catch the signal but we want to ignore it
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				perror("select");
				exit(-1);
			}

		}

		if (FD_ISSET(sock_fd, &set))
		{
			int msgsock = accept_connection(sock_fd); // wait for a client to connect
			printf("Got connection from client!\n");

			// handle the request with a new process
			if ((pid = fork()) < 0)
			{
				perror("Fork");
				exit(-1);
			}
			else if (pid == 0) // child
			{
				handle_request(msgsock, child_to_parent[1]);
				exit(0);
			}
			else
			{
				close(msgsock);
			}
		}
		if (FD_ISSET(child_to_parent[0], &set))
		{
			memset(buffer, 0, sizeof(buffer));
			ret = read(child_to_parent[0], buffer, sizeof(buffer));
			if (ret > 0)
			{
				printf("PARENT RECV: [%s]\n", buffer);
			}
			else
			{
				break;
			}
		}
	}

	close(sock_fd);
	exit (0);
}

