#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h> 
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>


#define MAXLINE 512


int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <Port> <Path_to_executable>\n", argv[0]);
        return -1;
    }

	int					listenfd, connfd;
	struct sockaddr_in	servaddr;
	struct sockaddr_in  cliaddr;
	char				buff[MAXLINE];
	time_t				ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port        = htons(atoi(argv[1]));

	bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	listen(listenfd, 20);
	for ( ; ; ) {
		int clilen = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);

		if (connfd < 0) {
			printf("Error accepting connection\n");
			return -1;
		}

		inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff));
		printf("New connection from %s:%d\n", buff, cliaddr.sin_port);
        signal(SIGCHLD, SIG_IGN);

        pid_t pid = fork();
		if (pid != 0) {
			continue;
		}

		pid = fork();
        if (pid == 0) {
            dup2(connfd, 1);
			dup2(connfd, 0);
            execvp(argv[2], argv + 2);
            fprintf(stderr, "Non valid executable.\n");
			close(connfd);
			_exit(0);
        } else {
            wait(NULL);
        }

		close(connfd);
	}

    return 0;

}