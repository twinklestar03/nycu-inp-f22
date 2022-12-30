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
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
        perror("sockfd"), exit(-1);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0)
        fail("inet_pton");

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        fail("connect");

    return sockfd;
}

inline void wrap_send(uint32_t data_seq, int sockfd, const void* buf, size_t len) {
    hdr_t hdr;
    bzero(&hdr, PACKET_SIZE);
    hdr.data_seq = data_seq;
    memcpy(hdr.data, buf, len);
#ifdef USE_CHECKSUM
    hdr.checksum = checksum(&hdr);
#endif
    // dump_hdr(&hdr);
    if (send(sockfd, &hdr, PACKET_SIZE, 0) < 0)
        fail("[cli] fail send");
}

int main(int argc, char *argv[]) {
    if (argc != 5)
        return -fprintf(stderr, "Usage: %s <path-to-read-files> <total-number-of-files> <port> <server-ip-address>", argv[0]);

    setvbufs();
    // signal(SIGCHLD, SIG_IGN);
    // signal(SIGPIPE, SIG_IGN);

    char* path = argv[1];
    uint32_t total = (uint32_t)atoi(argv[2]);
    uint16_t port = (uint16_t)atoi(argv[3]);
    char* ip = argv[4];

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

    int connfd = connect(ip, port);
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
        hdr_t res;
        while (recv(connfd, &res, sizeof(res), MSG_WAITALL) == sizeof(res)) {
#ifdef USE_CHECKSUM
            if (checksum(&res) != res.checksum) {
                dump_hdr(&res);
                continue;
            }
#endif
            s_res = res;
            if (res.data_seq == UINT32_MAX) {
                printf("[cli] sent done for (%u bytes)\n", comp_size);
                // usleep(100);
                exit(0);
                continue;
            }
        }
    };

    int r = 0;
    auto last_send = std::chrono::steady_clock::now();
    while (!data_map.empty()) {
        fprintf(stderr, "[cli] %d: %lu data packets left\n", ++r, data_map.size());
        for(auto [key, data] : data_map) if (!donebit[key])  {
            wrap_send(key, connfd, data.data, data.data_size);
            usleep(500);
        }
        if (data_map.size() > 100) {
            auto wait = std::chrono::milliseconds(SEND_TIME);
            std::this_thread::sleep_until(last_send + wait);
        }
        last_send = std::chrono::steady_clock::now();
        read_resps();
        for (size_t i = 0; i < DATA_SIZE*8; i++)
            if (donebit[i]) data_map.erase((uint32_t)i);
    }

    return 0;
}
