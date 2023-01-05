#include <array>
#include <arpa/inet.h>
#include <cstddef>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <filesystem>

#include "header.hpp"
#include "util.hpp"

int main(int argc, char *argv[]) {
    if(argc < 3) {
        return -fprintf(stderr, "usage: %s <data> <path>\n", argv[0]);
    }

    char* data = argv[1];
    char* path = argv[2];

    int fd = open(data, O_RDONLY);
    if (fd < 0) fail("open");
    auto orig_size = (uint32_t)lseek(fd, 0, SEEK_END);
    auto orig_data = new char[orig_size];
    lseek(fd, 0, SEEK_SET);
    read(fd, orig_data, orig_size);
    close(fd);

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

}
