/**
Multi-process chat server
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXDATASIZE 1024 // max buffer size
#define MAX_CONNECTIONS 100


#define ERROR_MSG "Server is full"

typedef struct {
    pthread_t thread_id;
    int msgsock;          // Socket for the connection - valid if > 0
    char ipstr[INET6_ADDRSTRLEN];
    int port;
} Connection;

Connection connections[MAX_CONNECTIONS] = {0};
int buffer_queue[2];
pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;

void get_peer_information(int s, Connection *client)
{
    socklen_t len;
    struct sockaddr_storage addr;
    char ipstr[INET6_ADDRSTRLEN];
    int port;

    len = sizeof addr;
    getpeername(s, (struct sockaddr*)&addr, &len);

    // deal with both IPv4 and IPv6:
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        port = ntohs(s->sin_port);
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    } else { // AF_INET6
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        port = ntohs(s->sin6_port);
        inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
    }

    strcpy(client->ipstr, ipstr);
    client->port = port;
}


Connection *next_available_slot(void)
{
    // returns the next available slot. Return NULL if no available slot anymore
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (connections[i].msgsock <= 0)
        {
            return &connections[i];
        }
    }
    return NULL;
}

int listen_on(int port)
{

	int sock_fd;
    struct sockaddr_in sa;

    // sock_fd is set to block
    sock_fd = socket(AF_INET,SOCK_STREAM,0);

	sa.sin_family = AF_INET;          /* communicate using internet address */
	sa.sin_addr.s_addr = INADDR_ANY; /* accept all calls                   */
	sa.sin_port = htons(port); /* this is port number                */

	int rc = bind(sock_fd, (struct sockaddr *)&sa, sizeof(sa)); /* bind address to socket   */
	if(rc == -1) { // Check for errors
		perror("bind");
		exit(1);
	}

	rc = listen (sock_fd, 5); /* listen for connections on a socket */
	if(rc == -1) { // Check for errors
		perror("listen");
		exit(1);
	}

	return sock_fd;
}

int accept_connection(int s) {
	int client_fd;
	struct sockaddr_in client_sock;


	socklen_t sz = sizeof(struct sockaddr_in);

	client_fd = accept(s, (struct sockaddr *) &client_sock, &sz);

    // set non-blocking of cmyfifolient_fd
    // fcntl(client_fd, F_SETFL, O_NONBLOCK);

	return client_fd;
}


int send_msg(int msgsock, char *msg)
{
    int ret;

    // write to sock
    ret = write(msgsock, msg, strlen(msg));

    return ret;
}


int broadcast(Connection *sender, char *msg)
{
    int sz;
    char buffer[MAXDATASIZE];

    // create sender
    if (sender != NULL)
    {
        sprintf(buffer, "[%s:%d]: %s", sender->ipstr, sender->port, msg);
    }
    else
    {
        strcpy(buffer, msg);
    }



    // lock the critical section
    pthread_mutex_lock(&conn_lock);


    sz = write(buffer_queue[1], buffer, strlen(buffer));
    if (sz < 0)
    {
        perror("broadcast_write");
    }

    // unlock the critical section
    pthread_mutex_unlock(&conn_lock);

    return sz;
}

void *broadcast_handler(void *args)
{
    ssize_t sz;
    char buffer[MAXDATASIZE];

    while (1)
    {
        // read message from pipe
        memset(buffer, 0, sizeof(buffer));
        sz = read(buffer_queue[0], buffer, MAXDATASIZE - 1);
        if (sz <= 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                continue;
            }
            else
            {
                break;
            }
        }

        // send message to all connected client
        else
        {
            for (int i = 0; i < MAX_CONNECTIONS; i++)
            {
                if (connections[i].msgsock > 0)
                {
                    // if failed to send message, then remove the client and cancel the thread
                    if (send_msg(connections[i].msgsock, buffer) < 0)
                    {
                        pthread_cancel(connections[i].thread_id);
                        close(connections[i].msgsock);
                    }
                }
            }
        }
    }

    // Oops, we should never run to this point
    printf("Exception!\n");
    close(buffer_queue[0]);
    close(buffer_queue[1]);

    pthread_exit((void *) -1);
}

void *request_handler(void *args)
{
    Connection *client = (Connection *)args;
    char buffer[MAXDATASIZE];
    ssize_t sz;


    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        sz = read(client->msgsock, buffer, MAXDATASIZE - 1);

        // handle non block request
        if (sz <= 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            printf("[%s:%d]: [%s]\n", client->ipstr, client->port, buffer);
            broadcast(client, buffer);
        }

    }

    // close the connection
    close(client->msgsock);

    // broadcast
    sprintf(buffer, "[%s:%d] lost", client->ipstr, client->port);
    printf("%s\n", buffer);
    broadcast(NULL, buffer);

    pthread_exit(NULL);
}


int main(int argc, char **argv)
{
    int sock_fd;
    int msgsock;
    Connection *client;
    pthread_t broadcast_thread_id;
    char buffer[MAXDATASIZE];

    // listen on port
    sock_fd = listen_on(atoi(argv[1]));
    if (sock_fd < 0)
    {
        perror("listen_on");
        exit(-1);// create sender
    }

    // open pipe for message buffering
    pipe(buffer_queue);

    // start broadcast handler
    pthread_create(&broadcast_thread_id, NULL, broadcast_handler, NULL);

    printf("\nThis is the server with pid %d listening on port %s\n", getpid(), argv[1]);

    // loop
    while (1)
    {
        // accept connection
        msgsock = accept_connection(sock_fd);

        // store connections
        if ((client = next_available_slot()) == NULL)
        {
            printf("%s\n", ERROR_MSG);
            if (send_msg(msgsock, ERROR_MSG) < 0)
            {
                perror("send_msg");
                close(msgsock);
            }
        }


        client->msgsock = msgsock;
        get_peer_information(msgsock, client);
        sprintf(buffer, "[%s:%d] connected", client->ipstr, client->port);
        printf("%s\n", buffer);
        broadcast(NULL, buffer);

        // create thread to handle request
        pthread_create(&client->thread_id, NULL, request_handler, client);

    }

    // We never expect to run our code here, so we should trap our code at this point and
    // wait for all threads exit
    printf("Server Error\n");
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (connections[i].msgsock > 0)
        {
            pthread_join(connections[i].thread_id, NULL);
        }
    }
}
