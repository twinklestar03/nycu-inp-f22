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

    // send data to server at constant Mega-bytes-per-second 
    printf("Sending data at %f MBps...\n", atof(argv[1]));
    float mega_byte_per_sec = atof(argv[1]);
    int packet_size = 2048 - 0x42;
    float packet_per_sec = mega_byte_per_sec * 1024 * 1024 / (packet_size + 0x42);
    float usec_per_packet = 1000000 / packet_per_sec;
    char buf[packet_size];
    memset(buf, 1, sizeof(buf));
    while (1) {
        // get the time differnece to get more accurate packet sending rate
        struct timeval t0, t1;
        gettimeofday(&t0, NULL);
        if (send(sock, buf, packet_size, 0) < 0) {
            perror("send");
            return 1;
        }
        gettimeofday(&t1, NULL);
        int usec = (t1.tv_sec - t0.tv_sec) * 1000000 + (t1.tv_usec - t0.tv_usec);
        if (usec < round(usec_per_packet)) {
            // convert us to ns
            struct timespec t = { 0, ((int) round(usec_per_packet - usec)) * 1000 };
            nanosleep(&t, NULL);
        }
    }
    return 0;
}