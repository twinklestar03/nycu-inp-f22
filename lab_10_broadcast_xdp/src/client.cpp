#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <chrono>
#include <bitset>
#include <vector>
#include <set>
#include <map>
#include <utility>
#include <algorithm>
#include <random>
#include <thread>

#include "header.hpp"
#include "util.hpp"
#include "dictionary.h"

#include "zstd.h"

std::mt19937 rng((int)std::chrono::steady_clock::now().time_since_epoch().count());

int connect(char* host, uint16_t port) {
    fprintf(stderr, "Connecting to %s:%d\n", host, port);
    int sockfd = socket(AF_INET, SOCK_RAW, 161);
    if (sockfd < 0) {
        fprintf(stderr, "socket error\n");
        perror("sockfd"), exit(-1);
    }

  
    return sockfd;
}

inline void wrap_send(char *host, uint32_t data_seq, int sockfd, const void* buf, size_t len) {
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", host);
        perror("inet_pton"), exit(-1);
    }

    // IP_HDRINCL
    int ip_hdrincl = 1;
    int bcast = 1;
    const int *val = &ip_hdrincl;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, val, sizeof(ip_hdrincl)) < 0) {
        fprintf(stderr, "setsockopt error\n");
        perror("setsockopt"), exit(-1);
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)) < 0) {
        close(sockfd);
        fail("setsockopt");
    }

    xdp_data_t xdp_data;
    bzero(&xdp_data, sizeof(xdp_data));
    
    // Filling in the data header
    xdp_data.data_hdr.data_seq = data_seq;
    memcpy(&xdp_data.data_hdr.data, buf, len);

    xdp_data.ip_hdr.ihl = 5;
    xdp_data.ip_hdr.version = 4;
    xdp_data.ip_hdr.tos = 0;
    xdp_data.ip_hdr.tot_len = htons(sizeof(iphdr) + sizeof(hdr_t));
    xdp_data.ip_hdr.id = htons(0);
    xdp_data.ip_hdr.frag_off = htons(0);
    xdp_data.ip_hdr.ttl = 255;
    xdp_data.ip_hdr.protocol = 161;
    xdp_data.ip_hdr.saddr = inet_addr("123.123.123.123");
    xdp_data.ip_hdr.daddr = inet_addr(host);
    xdp_data.ip_hdr.check = ip_checksum((unsigned short *)&xdp_data, sizeof(xdp_data));

    int ret = sendto(sockfd, &xdp_data, PACKET_SIZE, 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if (ret < 0) {
        fprintf(stderr, "sendto error\n");
        perror("sendto"), exit(-1);
    } 
    // fprintf(stderr, "Data sent!\n");
    
}

int main(int argc, char *argv[]) {
    if (argc != 4)
        return -fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <server-ip-address>", argv[0]);

    setvbufs();
    // signal(SIGCHLD, SIG_IGN);
    // signal(SIGPIPE, SIG_IGN);

    char* path = argv[1];
    uint32_t total = (uint32_t)atoi(argv[2]);
    // uint16_t port = (uint16_t)atoi(argv[3]);
    char* ip = argv[3];


    int bcast = 1;
    int connfd = connect(ip, 0);

    std::vector<file_t> files(total);

    std::map<uint32_t, data_t> data_map{};

    size_t total_bytes = 0;
    for(uint32_t idx = 0; idx < total; idx++) {
        char filename[256];
        sprintf(filename, "%s/%06d", path, idx);
        int filefd = open(filename, O_RDONLY);
        if (filefd < 0) fail("open");
        auto size = (uint32_t)lseek(filefd, 0, SEEK_END);
        auto data = new char[size];
        lseek(filefd, 0, SEEK_SET);
        read(filefd, data, size);
        close(filefd);

        files[idx] = {
            .filename = idx,
            .size = size,
            .init = {
                .filename = idx,
                .filesize = size,
            },
            .data = data,
        };
        printf("[cli] read '%s' (%u bytes)\n", filename, size);
        total_bytes += size;
    }

    auto orig_size = total * sizeof(init_t) + total_bytes;
    auto orig_data = new char[orig_size];
    {
        auto ptr = orig_data;
        for(auto& file: files) {
            memcpy(ptr, &file.init, sizeof(init_t));
            ptr += sizeof(init_t);
            memcpy(ptr, file.data, file.size);
            ptr += file.size;
        }
    }
    fprintf(stderr, "dictionary: %p (%d bytes)\n", dict, dict_size);
    auto buf_size = ZSTD_compressBound(orig_size);
    auto comp_data = new char[buf_size];
    uint32_t comp_size;
    {
        auto ctx = ZSTD_createCCtx();
        auto res = ZSTD_compress_usingDict(ctx, comp_data, buf_size, orig_data, orig_size, dict, (size_t)dict_size, COMPRESS_LEVEL);
        ZSTD_freeCCtx(ctx);
        if (ZSTD_isError(res)) {
            fprintf(stderr, "%s\n", ZSTD_getErrorName(res));
            exit(-1);
        }
        comp_size = (uint32_t)res;
    }

    fprintf(stderr, "[cli] total %lu -> %u bytes\n", total_bytes, comp_size);

    
    set_sockopt(connfd);

    data_t zero{
        .data_size = comp_size
    };
    data_map.emplace(0, data_t { .data_size = sizeof(data_t), .data = (char*)&zero });
    for(size_t i = 0; i * DATA_SIZE < comp_size; i++) {
        size_t offset = i * DATA_SIZE;
        data_map.emplace(
            i+1,
            data_t{
                .data_size = std::min(DATA_SIZE, comp_size - offset),
                .data = comp_data + offset,
            }
        );
    }

    hdr_t s_res;
    auto& donebit = *(std::bitset<DATA_SIZE*8>*)&s_res.data;

    auto read_resps = [&]() {
        xdp_data_t xdp_data;
        char buf[PACKET_SIZE];

        struct sockaddr_in servaddr;
        socklen_t sock_len = sizeof(servaddr);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
            fprintf(stderr, "inet_pton error for %s\n", ip);
            perror("inet_pton"), exit(-1);
        }

        while (recvfrom(connfd, buf, PACKET_SIZE, MSG_WAITALL, (struct sockaddr*) &servaddr, &sock_len) > 0) {
#ifdef USE_CHECKSUM
            if (checksum(&res) != res.checksum) {
                dump_hdr(&res);
                continue;
            }
#endif
            xdp_data = *(xdp_data_t*)buf;
            if (xdp_data.data_hdr.data_seq == UINT32_MAX) {
                printf("[cli] sent done for (%u bytes)\n", comp_size);
                // usleep(100);
                exit(0);
                continue;
            }
        }
    };

    int r = 0;
    int a = 3;
    auto last_send = std::chrono::steady_clock::now();
    while (data_map.size() > 0) {
        if (r != 1) {
            r = 1;
            fprintf(stderr, "[cli] %d: %lu data packets left\n", ++r, data_map.size());
            for(auto [key, data] : data_map) if (!donebit[key])  {
                wrap_send(ip, key, connfd, data.data, data.data_size);
                // usleep(500);
            }
            // if (data_map.size() > 100) {
            //     auto wait = std::chrono::milliseconds(SEND_TIME);
            //     std::this_thread::sleep_until(last_send + wait);
            // }
        }

        last_send = std::chrono::steady_clock::now();
        read_resps();
        for (size_t i = 0; i < DATA_SIZE*8; i++)
            if (donebit[i]) data_map.erase((uint32_t)i);
    }

    return 0;
}
