#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h> 
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <bitrate(MBps)>\n", argv[0]);
        return -1;
    }

    // create a tcp socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    // connect to server
    printf("Connecting to server...\n");
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10003);
    addr.sin_addr.s_addr = inet_addr("140.113.213.213");
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("Sending data at %f MBps...\n", atof(argv[1]));
    float target_byte_per_ms = atof(argv[1]) * 1000 * 1000 / 1000.0;
    int chunk_size = 1000;
    char chunk[chunk_size];
    int total_sent = 0;
    struct timeval start, end;

    memset(chunk, 0, chunk_size);
    gettimeofday(&start, NULL);
    while (1) {
        gettimeofday(&end, NULL);
        float elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
       
        if (elapsed >= 999.99) {
            printf("[>] Sent %d bytes in %f ms, %f MBpms\n", total_sent, elapsed, total_sent / elapsed * 1000 / 1000);
            total_sent = 0;
            gettimeofday(&start, NULL);
            continue;;
        }

        // sleep if the sending rate is too fast
        if (total_sent / elapsed > target_byte_per_ms) {
            struct timespec ts = {0, 100};
            nanosleep(&ts, NULL);
            continue;
        }

        int sent = send(sock, chunk, chunk_size, 0);
        if (sent < 0) {
            perror("send");
            return 1;
        }
        total_sent += sent + 0x32;
    }


    return 0;
}