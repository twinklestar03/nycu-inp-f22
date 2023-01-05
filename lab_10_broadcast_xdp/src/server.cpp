#include <array>
#include <arpa/inet.h>
#include <bitset>
#include <cstddef>
#include <climits>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>
#include <chrono>

#include "header.hpp"
#include "util.hpp"
#include "dictionary.h"

#include "zstd.h"

struct session_t {
    init_t file_metadata;
    uint32_t received_bytes = 0;
    bool is_complete = false;
};

using DATA_MAP_T = std::map<uint32_t, std::array<char,DATA_SIZE>>;

void do_reponse(int sock, hdr_t& hdr, char *host) {
#ifdef USE_CHECKSUM
    hdr.checksum = checksum(&hdr);
#endif
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, host, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", host);
        perror("inet_pton"), exit(-1);
    }

    xdp_data_t xdp_data;
    iphdr ip_hdr;
    bzero(&xdp_data, sizeof(xdp_data));
    bzero(&ip_hdr, sizeof(ip_hdr));
    
    // Filling in the data header
    memcpy(&xdp_data.data_hdr, &hdr, sizeof(hdr_t));

    xdp_data.ip_hdr.ihl = 5;
    xdp_data.ip_hdr.version = 4;
    xdp_data.ip_hdr.tos = 0;
    xdp_data.ip_hdr.tot_len = htons(sizeof(iphdr) + sizeof(hdr_t));
    xdp_data.ip_hdr.id = htons(0);
    xdp_data.ip_hdr.frag_off = htons(0);
    xdp_data.ip_hdr.ttl = 255;
    xdp_data.ip_hdr.protocol = 161;
    xdp_data.ip_hdr.saddr = inet_addr("123.123.123.125");
    xdp_data.ip_hdr.daddr = inet_addr(host);
    xdp_data.ip_hdr.check = ip_checksum((unsigned short *)&xdp_data, sizeof(xdp_data));

    int ret = sendto(sock, &xdp_data, PACKET_SIZE, 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if (ret < 0) {
        fprintf(stderr, "sendto error\n");
        perror("sendto"), exit(-1);
    } 

}

/*
 * Receive sender data and check if the packet is valid
 *
 * @return hdr_t* if the packet is valid, nullptr otherwise
 */
hdr_t* recv_sender_data(hdr_t* hdr, int sock, struct sockaddr_in* cin) {
    bzero(hdr, sizeof(hdr_t));
    socklen_t len = sizeof(&cin);

    // receive XDP packet
    char buf[PACKET_SIZE];
    if (recvfrom(sock, buf, PACKET_SIZE, MSG_WAITALL, (struct sockaddr*) &cin, &len) < 0) {
        fail("recvfrom");
    }

    // receive data from broadcast
    xdp_data_t data = *(xdp_data_t*)buf;
    memcpy(hdr, &data.data_hdr, sizeof(hdr_t));

    // dump_hdr(&data.data_hdr);

    // perform checksum
#ifdef USE_CHECKSUM
    if (hdr->checksum != checksum(hdr)) {
        dump_hdr(hdr);
        // TODO
        // do_reponse(sock, cin, hdr->data_seq);
        return nullptr;
    }
#endif

    return hdr;
}

int save_to_file(const char* path, uint32_t comp_size, DATA_MAP_T& data) {
    auto comp_data = new char[(1 + (comp_size-1) / DATA_SIZE) * DATA_SIZE];
    for(auto [seq, data_frag]: data)
        if (seq)
            memcpy(comp_data + (seq-1) * DATA_SIZE, &data_frag, DATA_SIZE);

#ifdef DUMP
    puts("BEGIN");
    write(1, comp_data, comp_size);
    puts("END");
#endif

    auto orig_size = ZSTD_getFrameContentSize(comp_data, comp_size);
    auto orig_data = new char[orig_size];
    auto ctx = ZSTD_createDCtx();
    ZSTD_decompress_usingDict(ctx, orig_data, orig_size, comp_data, comp_size, dict, (size_t)dict_size);
    ZSTD_freeDCtx(ctx);

    printf("[/] %u bytes -> %llu bytes\n", comp_size, orig_size);

    // // dump map
    // for (auto it = data.begin(); it != data.end(); ++it) {
    //     printf("[Chunk %d] ", it->first);
    //     for (int i = 0; i < PACKET_SIZE; ++i) {
    //         printf("%02x ", (uint8_t)it->second[i]);
    //     }
    // }
    // printf("\n");

    int cnt = 0;
    for(auto ptr = orig_data; ptr < orig_data + orig_size; cnt++) {
        auto init = (init_t*)ptr;
        ptr += sizeof(init_t);
        auto filesize = init->filesize;

        char filename[100];
        sprintf(filename, "%s/%06d", path, init->filename);
        printf("[/] File saved to %s (%d bytes)\n", filename, filesize);
        int fp = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fp == -1) fail("open");
        write(fp, ptr, filesize);
        ptr += filesize;
        close(fp);
    }

    delete [] comp_data;
    delete [] orig_data;
    data.clear();
    return cnt;
}

int main(int argc, char *argv[]) {
    if(argc < 4) {
        return -fprintf(stderr, "usage: %s <path-to-store-files> <total-number-of-files> <broadcast-address>\n", argv[0]);
    }

    char* path = argv[1];
    int total = atoi(argv[2]);
    // auto port = (uint16_t)atoi(argv[3]);
    auto ip = argv[3];
    struct sockaddr_in broadcast_in;

    setvbufs();
    
    bzero(&broadcast_in, sizeof(broadcast_in));
    broadcast_in.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &broadcast_in.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s\n", ip);
        perror("inet_pton"), exit(-1);
    }

    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_RAW, 161)) < 0)
        fail("socket");

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

    // // receive data from broadcast
    // char buf[PACKET_SIZE];
    // socklen_t len = sizeof(sin);
    // recvfrom(sockfd, buf, PACKET_SIZE, MSG_WAITALL, (struct sockaddr*) &sin, &len);

    // xdp_data_t data = *(xdp_data_t*)buf;

    // dump_hdr(&data.data_hdr);


    uint32_t comp_size = UINT_MAX, recv_size = 0;
    DATA_MAP_T data_map;
    hdr_t recv_hdr;

    auto last_send = std::chrono::steady_clock::now();
    bool keep = false;
    int r = 0;
    hdr_t send_hdr{
        .data_seq = 0
    };
    auto& recvbit = *(std::bitset<DATA_SIZE*8>*)&send_hdr.data;

    while (comp_size == UINT_MAX or recv_size < comp_size) {
        if (recv_sender_data(&recv_hdr, sockfd, &broadcast_in) != nullptr) {
            // session initialization
            auto data_seq = recv_hdr.data_seq;
            recvbit[data_seq] = 1;
            if (data_seq == 0) {
                // create a new session
                data_t tmp;
                memcpy(&tmp, recv_hdr.data, sizeof(data_t));
                comp_size = (uint32_t)tmp.data_size;
                printf("[/] [ChunkID=%d] initiation %d bytes\n", data_seq, comp_size);
            } else {
                // save the data chunk
                if (DEBUG) {
                    printf("[/] [ChunkID=%d] Received data chunk from %s:%d\n",
                        data_seq,
                        inet_ntoa(broadcast_in.sin_addr),
                        ntohs(broadcast_in.sin_port)
                    );
                }
                if (!data_map.count(data_seq)) {
                    auto& data_frag = data_map[data_seq];
                    memcpy(&data_frag, recv_hdr.data, DATA_SIZE);
                    recv_size += DATA_SIZE;
                }
            }
        }

        // send a ACK to the client
        if (!keep and (comp_size / DATA_SIZE) - recvbit.count() < 100)
            keep = true;
        auto now = std::chrono::steady_clock::now();
        if (now - last_send > std::chrono::milliseconds(keep ? 10 : SEND_TIME)) {
            last_send = now;
            fprintf(stderr, "[/] %2.2f send %lu ack\n", ++r / double(SEND_WAIT), recvbit.count());
            do_reponse(sockfd, send_hdr, ip);
        }
    }

    auto write = save_to_file(path, comp_size, data_map);
    fprintf(stderr, "[/] save %d / %d files\n", write, total);

    // All data recvived, send FIN
    fprintf(stderr, "[/] Received all data, sending FIN\n");

    send_hdr.data_seq = UINT32_MAX;
    while (true) {
        if (recv_sender_data(&recv_hdr, sockfd, &broadcast_in) == nullptr)
            continue;
        
        do_reponse(sockfd, send_hdr, ip);
        fprintf(stderr, "[/] Received all data, sending FIN\n");
        usleep(100000);
    }

    close(sockfd);

    return 0;
}
