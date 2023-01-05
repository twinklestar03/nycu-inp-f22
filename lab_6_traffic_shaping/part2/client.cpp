#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string>
#include <map>
#include <pthread.h>


#define WORKERS 20

int    cmdfd;
char   recvline[1024];

void handler(int s) {
    send(cmdfd, "/reports", 8, 0);
    recv(cmdfd, recvline, 1024, 0);
    printf("%s", recvline);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <host> <port>\n", argv[0]);
        return -1;
    }

    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    printf("Connecting all clients to server...\n");
    struct sockaddr_in      addr;
    struct epoll_event      ev;
    struct epoll_event      *events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * WORKERS);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(argv[2]));
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    cmdfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cmdfd < 0) {
        printf("Error creating socket\n");
        return -1;
    }
    if (connect(cmdfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        printf("Error connecting to server\n");
        return -1;
    }
    send(cmdfd, "/reset", 6, 0);
    recv(cmdfd, recvline, 1024, 0);
    
    addr.sin_port = htons(atoi(argv[2]) + 1);
    int epfd = epoll_create(WORKERS);
    for (int i = 0; i < WORKERS; i++) {
        int socks = socket(AF_INET, SOCK_STREAM, 0);
        if (socks < 0) {
            printf("Error creating socket\n");
            return -1;
        }
        if (connect(socks, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            printf("Error connecting to server\n");
            return -1;
        }
        ev.events = EPOLLOUT;
        ev.data.fd = socks;
        epoll_ctl(epfd, EPOLL_CTL_ADD, socks, &ev);
    }
    
    char buf[65535] = {'A'};
    while(1) {
        int nfds = epoll_wait(epfd, events, WORKERS, -1);
        for (int i = 0; i < nfds; i++) {
            send(events[i].data.fd, buf, 65534, MSG_DONTWAIT);
        }
    }

    return 0;
}