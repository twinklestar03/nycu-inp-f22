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

typedef struct user_t {
    char                    username[50];
    struct sockaddr_in      cliaddr;
} user_t;
std::map<int, user_t>   connections_map;

int handle_read(epoll_event* event, int nfds, int to_handle);

int broadcast(epoll_event* event, int nfds, char *msg, int len);

void sig_handler(int signum){

}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <Port>\n", argv[0]);
        return -1;
    }

    int                     listenfd, connfd;
    int                     epfd;
    int                     online_users = 0;
    struct sockaddr_in      servaddr;
    struct sockaddr_in      cliaddr;
    struct epoll_event      ev;
    struct epoll_event      *events = (struct epoll_event*) malloc(sizeof(struct epoll_event) * MAX_EVENTS);
    
    struct sigaction        sigac   = {sig_handler};
    sigaction(SIGPIPE, &sigac, NULL);
    
    int reuse = 1;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1]));

    bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
    listen(listenfd, MAX_EVENTS);

    epfd = epoll_create(MAX_EVENTS);
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    while (1) {
       
        int nfds = epoll_wait(epfd, events, 20, -1);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listenfd) {    // Newly connection
                int clilen = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr*) &cliaddr, (socklen_t*) &clilen);
                if (connfd < 0) {
                    printf("Error accepting connection");
                    return -1;
                }

                char buff[0x90] = {0};
                inet_ntop(AF_INET, &cliaddr.sin_addr, buff, sizeof(buff));
		        printf("client connected from %s:%d\n", buff, cliaddr.sin_port);

                // Add the new connection to the epoll
                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                //fcntl(connfd, F_SETFL, O_NONBLOCK);
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);


                // generate user information and add to map
                user_t user_info;
                char name[50];
                sprintf(name, "happy-user-%d", rand() % 100000);
                strncpy(user_info.username, name, 50);
                user_info.cliaddr = cliaddr;
                connections_map[connfd] = user_info;

                // send banner
                // 2022-10-05 15:45:00 *** Welcome to the simple CHAT server
                // 2022-10-05 15:45:00 *** Total 2 users online now. Your name is <becoming babirusa>
                char banner[1024], timestamp[50];
                time_t now = time(0);
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
                sprintf(banner, "%s *** Welcome to the simple CHAT server\n%s *** Total %d users online now. Your name is <%s>\n", timestamp, timestamp, online_users++, name);
                send(connfd, banner, strlen(banner), MSG_DONTWAIT);

                // broadcast messages
                // 2022-10-05 21:50:08 *** User <fabulous caudata> has just landed on the server
                char msg[1024];
                sprintf(msg, "%s *** User <%s> has just landed on the server\n", timestamp, name);
                broadcast(events, nfds, msg, strlen(msg));
            }
            else if (events[i].events & EPOLLIN) {  // Readable
                connfd = events[i].data.fd;
                if (connfd < 0) {
                    continue;
                }
                if (handle_read(events, nfds, connfd) < 0) {
                    // 2022-10-05 16:23:05 *** User <flayed jabiru> has left the server
                    char msg[1024], timestamp[50];
                    time_t now = time(0);
                    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
                    sprintf(msg, "%s *** User <%s> has left the server\n", timestamp, connections_map[connfd].username);
                    broadcast(events, nfds, msg, strlen(msg));

                    // client disconnected
                    char buff[0x90] = {0};
                    inet_ntop(AF_INET, &connections_map[connfd].cliaddr.sin_addr, buff, sizeof(buff));
                    printf("client %s:%d disconnected\n", buff, connections_map[connfd].cliaddr.sin_port);

                    online_users--;
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL);
                    close(connfd);
                }
            }            
        }
    }

    free(events);

    return 0;
}

int handle_read(epoll_event* event, int nfds, int to_handle) {
    char buff[512]   = {0};
    int nread;

    nread = read(to_handle, buff, 512);
    if (nread == 0) {   // client has disconnected.
        return -1;
    }

    // if slash command, handle it
    if (buff[0] == '/') {
        if (strncmp(buff, "/name", 5) == 0 && strlen(buff) > 7) {
            char *newname = buff + 6;
            char oldname[50] = {0};
            strncpy(oldname, connections_map[to_handle].username, 50);
            newname[strlen(newname) - 1] = '\0';
            strncpy(connections_map[to_handle].username, newname, 50);

            printf("[DEBUG]old: %s name: %s\n", oldname, newname);

            // 2022-10-05 16:05:34 *** Nickname changed to <hello, world!>
            char msg[1024], timestamp[50];
            time_t now = time(0);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            sprintf(msg, "%s *** Nickname changed to <%s>\n", timestamp, connections_map[to_handle].username);
            send(to_handle, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);
            // 2022-10-05 16:06:03 *** User <jubilant cob> renamed to <hello, world!>
            sprintf(msg, "%s *** User <%s> renamed to <%s>\n", timestamp, oldname, connections_map[to_handle].username);
            broadcast(event, nfds, msg, strlen(msg));
        } else if (strncmp(buff, "/who", 4) == 0) {
            // Give a list of all users with their IP address and port
            /*
                --------------------------------------------------
                uncivil mayfly       140.113.1.2:55360
                browny bison         140.113.1.2:59476
                * hello, world!        140.113.1.2:59478
                --------------------------------------------------
            */

           // iterate through connections_map and dump info
            std::map<int, user_t>::iterator it;
            char msg[1024];
            sprintf(msg, "--------------------------------------------------\n");
            send(to_handle, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);
            for (it = connections_map.begin(); it != connections_map.end(); it++) {
                char ip[50];
                inet_ntop(AF_INET, &it->second.cliaddr.sin_addr, ip, sizeof(ip));
                if (it->first == to_handle) {
                    sprintf(msg, "* %-30s\t%s:%d\n", it->second.username, ip, it->second.cliaddr.sin_port);
                } else {
                    sprintf(msg, "  %-30s\t%s:%d\n", it->second.username, ip, it->second.cliaddr.sin_port);
                }
                send(to_handle, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);
            }
            sprintf(msg, "--------------------------------------------------\n");
            send(to_handle, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);
        } else {
            // 2022-10-05 16:13:40 *** Unknown or incomplete command </name>
            char msg[1024], timestamp[50];
            time_t now = time(0);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            buff[nread - 1] = '\0';
            sprintf(msg, "%s *** Unknown or incomplete command <%s>\n", timestamp, buff);
            send(to_handle, msg, strlen(msg), MSG_NOSIGNAL | MSG_DONTWAIT);
        }
    }
    else {  // if message, broadcast it
        char msg[1024], timestamp[50];
        time_t now = time(0);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
        // 2022-10-05 16:20:12 <flayed jabiru> hey guys, how's everything?
        sprintf(msg, "%s <%s> %s\n", timestamp, connections_map[to_handle].username, buff);
        broadcast(event, nfds, msg, strlen(msg));
    }
    
    //write(connfd, buff, nread);
    return 0;
}

int broadcast(epoll_event* event, int nfds, char *msg, int len) {
    std::map<int, user_t>::iterator it;
    for (it = connections_map.begin(); it != connections_map.end(); it++) {
        send(it->first, msg, len, MSG_DONTWAIT);
    }
    return 0;
}
