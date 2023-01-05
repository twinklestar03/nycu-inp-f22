// Create a TCP server using epoll multiplexing
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


#define BUFFER_SIZE 512
#define MAX_EVENTS  1500


std::map<int, bool>   is_sink;

uint32_t current_sink = 0;

uint64_t bandwidth_counter = 0;

struct timeval start_time;

void sig_handler(int signum){

}


int handle_read(epoll_event* event, int nfds, int to_handle, bool is_sink);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <Command-Port>\n", argv[0]);
        return -1;
    }

    int                     cmdfd, sinkfd, connfd;
    int                     epfd;
    int                     online_users = 0;
    struct sockaddr_in      cmdaddr;
    struct sockaddr_in      sinkaddr;
    struct sockaddr_in      cliaddr;
    struct epoll_event      ev;
    struct epoll_event      *events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    
    struct sigaction        sigac   = {sig_handler};
    sigaction(SIGPIPE, &sigac, NULL);
    
    int reuse = 1;
    cmdfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(cmdfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    sinkfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sinkfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    

    bzero(&cmdaddr, sizeof(cmdaddr));
    cmdaddr.sin_family = AF_INET;
    cmdaddr.sin_addr.s_addr = INADDR_ANY;
    cmdaddr.sin_port = htons(atoi(argv[1]));

    bzero(&sinkaddr, sizeof(sinkaddr));
    sinkaddr.sin_family = AF_INET;
    sinkaddr.sin_addr.s_addr = INADDR_ANY;
    sinkaddr.sin_port = htons(atoi(argv[1]) + 1);

    bind(cmdfd, (struct sockaddr*) &cmdaddr, sizeof(cmdaddr));
    bind(sinkfd, (struct sockaddr*) &sinkaddr, sizeof(sinkaddr));

    listen(cmdfd, MAX_EVENTS);
    listen(sinkfd, MAX_EVENTS);

    epfd = epoll_create(MAX_EVENTS);
    ev.events = EPOLLIN;
    ev.data.fd = cmdfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, cmdfd, &ev);
    ev.data.fd = sinkfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sinkfd, &ev);

    gettimeofday(&start_time, NULL);
    printf("%ld: Server started at %d, please sink data at %d\n", 
        start_time.tv_sec+start_time.tv_usec*1000000, atoi(argv[1]), atoi(argv[1]) + 1);

    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == cmdfd) {   // Command Fd
                int clilen = sizeof(cliaddr);
                connfd = accept(cmdfd, (struct sockaddr*) &cliaddr, (socklen_t*) &clilen);
                if (connfd < 0) {
                    printf("Error accepting connection");
                    return -1;
                }

                printf("New command connection arrived!\n");

                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
                is_sink[connfd] = false;
            } else if (events[i].data.fd == sinkfd) {
                int clilen = sizeof(cliaddr);
                connfd = accept(sinkfd, (struct sockaddr*) &cliaddr, (socklen_t*) &clilen);
                if (connfd < 0) {
                    printf("Error accepting connection");
                    return -1;
                }

                printf("New sink connection arrived!\n");

                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);

                is_sink[connfd] = true;
                current_sink++;
            } else if (events[i].events & EPOLLIN) {  // Readable
                connfd = events[i].data.fd;
                if (connfd < 0) {
                    continue;
                }
                if (handle_read(events, nfds, connfd, is_sink[connfd]) < 0) {
                    if (is_sink[connfd]) {
                        current_sink--;
                        is_sink[connfd] = false;
                    }
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                    close(connfd);
                }
            }            
        }
    }

    free(events);

    return 0;
}

int handle_read(epoll_event* event, int nfds, int to_handle, bool is_sink) {
    char received[512]   = {0};
    int nread;
    nread = read(to_handle, received, 512);

    if (nread < 0) {
        printf("Error reading from socket");
        return -1;
    } else if (nread == 0) {    // client disconnected
        return -1;
    }

    if (!is_sink) {
        if (received[0] != '/') {   // invalid command
            return 0;
        }

        struct timeval tv;
        gettimeofday(&tv, NULL);
        char buf[256] = {0};
        if (strncmp(received, "/reset", 6) == 0) {
            // <time> RESET <counter-value-before-reset>\n
            gettimeofday(&start_time, NULL);
            sprintf(buf, "%ld RESET %lu\n", tv.tv_sec * 1000000 + tv.tv_usec, bandwidth_counter);
            bandwidth_counter = 0;
        } else if (strncmp(received, "/ping", 5) == 0) {
            // <time> PONG\n
            sprintf(buf, "%ld PONG\n", tv.tv_sec * 1000000 + tv.tv_usec);
        } else if (strncmp(received, "/report", 7) == 0) {
            // <time> REPORT <counter-value> <elapsed-time>s <measured-megabits-per-second>Mbps\n
            double elpased_time = (tv.tv_sec - start_time.tv_sec) + (tv.tv_usec - start_time.tv_usec) / 1000000;
            sprintf(buf, "%ld REPORT %lu %fs %lfMbps\n", 
                tv.tv_sec * 1000000 + tv.tv_usec, bandwidth_counter, elpased_time, 8.0 * bandwidth_counter / 1000000.0 / elpased_time);

        } else if (strncmp(received, "/clients", 8) == 0) {
            // <time> CLIENTS <number-of-connected-data-sink-connections>\n
            sprintf(buf, "%ld CLIENTS %d\n", tv.tv_sec * 1000000 + tv.tv_usec, current_sink);
        }

        send(to_handle, buf, strlen(buf), 0);
        return 0;
    }

    // Data Sink
    bandwidth_counter += nread;
    return 0;
}
