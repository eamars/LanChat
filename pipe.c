#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define NUM_CHILD 10
#define MAX_MSG_SZ 1024


void child_code(int *parent_to_child, int *child_to_parent)
{
	int pid;

	close(child_to_parent[0]); // close pipe from parent to child
	pid = getpid();
	write(child_to_parent[1], &pid, sizeof(pid));
	printf("SENDING %d TO PARENT\n", pid);

	// listen to message from parent
	close(parent_to_child[1]);
	fd_set set;
	int ret;
	char msg[MAX_MSG_SZ];


	FD_ZERO(&set);
	FD_SET(parent_to_child[0], &set);
	while (1)
	{
		ret = select(FD_SETSIZE, &set, NULL, NULL, NULL);
		if (ret > 0)
		{
			ret = read(parent_to_child[0], msg, MAX_MSG_SZ);
			if (ret > 0)
			{
				printf("CHILD RECV: [%s]\n", msg);
			}
			else
			{
				break;
			}
		}
		else
		{
			perror("select");
		}
	}

}

int main(void){
	int i;
	int sz;
	int child_to_parent[2];
	int parent_to_child[NUM_CHILD][2];
	int pid;
	char msg[MAX_MSG_SZ];


	signal(SIGCHLD, SIG_IGN);

	// create child to parent pipe
	if (pipe(child_to_parent) < 0)
	{
		perror("pipe");
		exit(-1);
	}

	// fork child process
	for (i = 0; i < NUM_CHILD; i++)
	{
		// prepare for pipe that from parent to child
		if (pipe(parent_to_child[i]) < 0)
		{
			perror("pipe");
			exit(-1);
		}
		printf("PIPE [%d, %d]\n", parent_to_child[i][0], parent_to_child[i][1]);

		if ((pid = fork()) < 0)
		{
			perror("fork");
			exit(-1);
		}
		else if (pid == 0)
		{
			// child code

			child_code(parent_to_child[i], child_to_parent);
			exit(0);
		}
		else
		{
			// parent code
		}
	}

	// send to child
	close(child_to_parent[1]); // close pipe from parent to child

	for (i = 0; i < NUM_CHILD; i++)
	{
		close(parent_to_child[i][0]);
		sprintf(msg, "PARENT TO CHILD %d", i);
		write(parent_to_child[i][1], msg, strlen(msg));
	}
	sleep(5);

	// recv from child
	fd_set set;
	int ret;
	FD_ZERO(&set);
	FD_SET(child_to_parent[0], &set);
	while (1)
	{
		ret = select(FD_SETSIZE, &set, NULL, NULL, NULL);
		if (ret > 0)
		{
			ret = read(child_to_parent[0], &pid, sizeof(pid));
			if (ret > 0)
			{
				printf("PARENT RECV: [%d]\n", pid);
			}
			else
			{
				break;
			}
		}
		else
		{
			perror("select");
		}
	}

	return 0;
}

