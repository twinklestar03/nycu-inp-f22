#pragma once

#include <cstdlib>
#include <stdio.h>
#include <errno.h>
#include <cstddef>
#include <stdint.h>
#include <sys/socket.h>

#include "header.hpp"

inline void fail(const char* s) {
    if (errno) perror(s);
    else fprintf(stderr, "%s: unknown error", s);
#ifndef DEBUG
    exit(-1);
#endif
}

inline void setvbufs() {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
}

inline uint32_t adler32(const void *_data, size_t len)
/*
    where data is the location of the data in physical memory and
    len is the length of the data in bytes
*/
{
    auto data = (const uint8_t*)_data;
    uint32_t a = 1, b = 0;
    size_t index;

    // Process each byte of the data in order
    for (index = 0; index < len; ++index)
    {
        a = (a + data[index]) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }

    return (b << 16) | a;
}

#ifdef USE_CHECKSUM
inline uint32_t checksum(const sender_hdr_t* hdr) {
    uint32_t value = adler32(hdr->data, DATA_SIZE);
    return value ^ hdr->sess_seq.id;
}

inline uint32_t checksum(const response_hdr_t* hdr) {
    return hdr->sess_seq.id ^ hdr->flag ^ RES_MAGIC;
}
#endif

/*
inline void dump_hdr(const struct sender_hdr_t* hdr) {
#ifdef USE_CHECKSUM
    printf("[*] Checksum, seq=%d, recv_chk=%x, my_chk=%x\n",
        hdr->sess_seq.seq, hdr->checksum, checksum(hdr)
    );
#endif
    // dump data as hex
    printf("[*] Data: ");
    for (size_t i = 0; i < DATA_SIZE; i++) {
        printf("%02x ", (uint8_t)hdr->data[i]);
    }
    printf("\n");
}

inline void dump_hdr(const struct response_hdr_t* hdr) {
#ifdef USE_CHECKSUM
    printf("[*] Checksum, seq=%d, recv_chk=%x, my_chk=%x, flag=%d\n",
        hdr->sess_seq.seq, hdr->checksum, checksum(hdr), hdr->flag
    );
#endif
}
*/

inline void set_sockopt(int sockfd) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        fail("setsockopt(SO_RCVTIMEO)");
    size_t Ssize = 2 * TOTAL_SIZE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &Ssize, sizeof(Ssize)) < 0)
        fail("setsockopt(SO_SNDBUF)");
    size_t Rsize = 16 * 1024 * TOTAL_SIZE;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &Rsize, sizeof(Rsize)) < 0)
        fail("setsockopt(SO_RCVBUF)");
}
