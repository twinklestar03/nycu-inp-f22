#pragma once
#include <cstdint>
#include <unistd.h>

// #define DEBUG

#ifdef DEBUG
#undef DEBUG
#define DEBUG DEBUG
constexpr bool DEBUG = 1;
#else
constexpr bool DEBUG = 0;
#define printf(...) 1
#endif
// #define USE_CHECKSUM

constexpr int COMPRESS_LEVEL = 12;
constexpr uint32_t MOD_ADLER = 65521;

struct init_t {
    uint32_t filename;
    uint32_t filesize;
};

struct file_t {
    uint32_t filename;
    size_t   size;
    init_t   init;
    char*    data;
};

struct data_t {
    size_t   data_size;
    char*    data;
};

constexpr size_t TOTAL_SIZE = 1200;
constexpr size_t HEADER_SIZE = 28;
constexpr size_t DATA_SIZE = TOTAL_SIZE - HEADER_SIZE - sizeof(uint32_t)
#ifdef USE_CHECKSUM
    - sizeof(uint32_t);
#else
    ;
#endif
struct hdr_t {
    uint32_t data_seq = 0;
#ifdef USE_CHECKSUM
    uint32_t checksum = 0;
#endif
    char     data[DATA_SIZE] = { 0 };
};
constexpr size_t PACKET_SIZE = sizeof(hdr_t);

constexpr size_t BANDWIDTH = 10 * 1024 * 1024 / 8;
constexpr size_t WINDOW_SIZE = 64;

constexpr int SEND_WAIT = 2;
constexpr int SEND_TIME = 1000 / SEND_WAIT;
